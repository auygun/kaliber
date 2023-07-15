#include "engine/audio/mixer_input.h"

#include "base/log.h"
#include "base/thread_pool.h"
#include "engine/audio/audio_bus.h"
#include "engine/audio/audio_mixer.h"

using namespace base;

namespace eng {

MixerInput::MixerInput() {
  DLOG(1) << "MixerInput created: " << uintptr_t(this);
}

MixerInput::~MixerInput() {
  DLOG(1) << "Destroying MixerInput: " << uintptr_t(this);
}

// static
std::shared_ptr<MixerInput> MixerInput::Create() {
  return std::shared_ptr<MixerInput>(new MixerInput());
}

void MixerInput::SetAudioBus(std::shared_ptr<AudioBus> audio_bus) {
  if (playing_)
    pending_audio_bus_ = audio_bus;
  else
    audio_bus_ = audio_bus;
}

void MixerInput::Play(AudioMixer* mixer, bool restart) {
  if (!mixer->IsAudioEnabled()) {
    if (!playing_ && end_cb_)
      end_cb_();
    return;
  }

  // If already playing check if stream position needs to be reset.
  if (playing_) {
    if (restart)
      flags_.fetch_or(kStopped, std::memory_order_relaxed);

    if (flags_.load(std::memory_order_relaxed) & kStopped)
      restart_cb_ = [&, mixer, restart]() -> void { Play(mixer, restart); };

    return;
  }

  if (restart || audio_bus_->EndOfStream()) {
    src_index_ = 0;
    accumulator_ = 0;
    audio_bus_->ResetStream();
  }

  playing_ = true;
  flags_.fetch_and(~kStopped, std::memory_order_relaxed);
  mixer->AddInput(shared_from_this());
}

void MixerInput::Stop() {
  if (playing_) {
    restart_cb_ = nullptr;
    flags_.fetch_or(kStopped, std::memory_order_relaxed);
  }
}

void MixerInput::SetLoop(bool loop) {
  if (loop)
    flags_.fetch_or(kLoop, std::memory_order_relaxed);
  else
    flags_.fetch_and(~kLoop, std::memory_order_relaxed);
}

void MixerInput::SetSimulateStereo(bool simulate) {
  if (simulate)
    flags_.fetch_or(kSimulateStereo, std::memory_order_relaxed);
  else
    flags_.fetch_and(~kSimulateStereo, std::memory_order_relaxed);
}

void MixerInput::SetResampleStep(size_t value) {
  step_.store(value + 100, std::memory_order_relaxed);
}

void MixerInput::SetAmplitude(float value) {
  amplitude_.store(value, std::memory_order_relaxed);
}

void MixerInput::SetMaxAmplitude(float value) {
  max_amplitude_.store(value, std::memory_order_relaxed);
}

void MixerInput::SetAmplitudeInc(float value) {
  amplitude_inc_.store(value, std::memory_order_relaxed);
}

void MixerInput::SetEndCallback(base::Closure cb) {
  end_cb_ = std::move(cb);
}

bool MixerInput::IsStreamingInProgress() const {
  return streaming_in_progress_.load(std::memory_order_relaxed);
}

void MixerInput::SetPosition(size_t index, int accumulator) {
  src_index_ = index;
  accumulator_ = accumulator;
}

bool MixerInput::OnMoreData(bool loop) {
  if (streaming_in_progress_.load(std::memory_order_acquire))
    return false;

  streaming_in_progress_.store(true, std::memory_order_relaxed);
  audio_bus_->SwapBuffers();
  ThreadPool::Get().PostTask(
      HERE,
      [&, loop]() {
        audio_bus_->Stream(loop);
        streaming_in_progress_.store(false, std::memory_order_release);
      },
      true);
  return true;
}

void MixerInput::OnRemovedFromMixer() {
  DCHECK(!streaming_in_progress_.load(std::memory_order_relaxed));
  DCHECK(playing_);
  playing_ = false;

  if (pending_audio_bus_) {
    audio_bus_ = pending_audio_bus_;
    pending_audio_bus_.reset();
  }

  if (end_cb_)
    end_cb_();
  if (restart_cb_) {
    restart_cb_();
    restart_cb_ = nullptr;
  }
}

}  // namespace eng
