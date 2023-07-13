#include "engine/audio/mixer_input.h"

#include "base/log.h"
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

void MixerInput::SetAudioBus(std::shared_ptr<AudioBus> bus) {
  if (streaming)
    pending_audio_bus = bus;
  else
    audio_bus = bus;
}

void MixerInput::Play(AudioMixer* mixer, bool restart) {
  if (!mixer->IsAudioEnabled())
    return;

  // If already streaming check if stream position needs to be reset.
  if (streaming) {
    if (restart)
      flags.fetch_or(kStopped, std::memory_order_relaxed);

    if (flags.load(std::memory_order_relaxed) & kStopped)
      restart_cb = [&, mixer, restart]() -> void { Play(mixer, restart); };

    return;
  }

  if (restart || audio_bus->EndOfStream()) {
    src_index = 0;
    accumulator = 0;
    audio_bus->ResetStream();
  }

  streaming = true;
  flags.fetch_and(~kStopped, std::memory_order_relaxed);
  mixer->AddInput(shared_from_this());
}

void MixerInput::Stop() {
  if (streaming) {
    restart_cb = nullptr;
    flags.fetch_or(kStopped, std::memory_order_relaxed);
  }
}

void MixerInput::SetLoop(bool loop) {
  if (loop)
    flags.fetch_or(kLoop, std::memory_order_relaxed);
  else
    flags.fetch_and(~kLoop, std::memory_order_relaxed);
}

void MixerInput::SetSimulateStereo(bool simulate) {
  if (simulate)
    flags.fetch_or(kSimulateStereo, std::memory_order_relaxed);
  else
    flags.fetch_and(~kSimulateStereo, std::memory_order_relaxed);
}

void MixerInput::SetResampleStep(size_t value) {
  step.store(value + 100, std::memory_order_relaxed);
}

void MixerInput::SetAmplitude(float value) {
  amplitude.store(value, std::memory_order_relaxed);
}

void MixerInput::SetMaxAmplitude(float value) {
  max_amplitude.store(value, std::memory_order_relaxed);
}

void MixerInput::SetAmplitudeInc(float value) {
  amplitude_inc.store(value, std::memory_order_relaxed);
}

void MixerInput::SetEndCallback(base::Closure cb) {
  end_cb = std::move(cb);
}

}  // namespace eng
