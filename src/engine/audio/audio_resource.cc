#include "audio_resource.h"

#include "../../base/log.h"
#include "../sound.h"
#include "audio.h"
#include "audio_sample.h"

using namespace base;

namespace eng {

AudioResource::AudioResource(Audio* audio)
    : sample_(std::make_shared<AudioSample>()), audio_(audio) {}

AudioResource::~AudioResource() {
  sample_->flags.fetch_or(AudioSample::kStopped, std::memory_order_relaxed);
}

void AudioResource::Play(std::shared_ptr<Sound> sound,
                         float amplitude,
                         bool reset_pos) {
  AudioSample* sample = sample_.get();

  if (sample->active) {
    if (reset_pos)
      sample_->flags.fetch_or(AudioSample::kStopped, std::memory_order_relaxed);

    if (reset_pos ||
        sample->flags.load(std::memory_order_relaxed) & AudioSample::kStopped) {
      Closure ocb = sample_->end_cb;
      SetEndCallback([&, sound, amplitude, reset_pos, ocb]() -> void {
        Play(sound, amplitude, reset_pos);
        SetEndCallback(ocb);
      });
    }

    return;
  }

  if (reset_pos) {
    sample->src_index = 0;
    sample->accumulator = 0;
    sound->ResetStream();
  }

  sample->active = true;
  sample_->flags.fetch_and(~AudioSample::kStopped, std::memory_order_relaxed);
  sample->sound = sound;
  if (amplitude >= 0)
    sample->amplitude = amplitude;

  audio_->Play(sample_);
}

void AudioResource::Stop() {
  if (sample_->active)
    sample_->flags.fetch_or(AudioSample::kStopped, std::memory_order_relaxed);
}

void AudioResource::SetLoop(bool loop) {
  if (loop)
    sample_->flags.fetch_or(AudioSample::kLoop, std::memory_order_relaxed);
  else
    sample_->flags.fetch_and(~AudioSample::kLoop, std::memory_order_relaxed);
}

void AudioResource::SetSimulateStereo(bool simulate) {
  if (simulate)
    sample_->flags.fetch_or(AudioSample::kSimulateStereo,
                            std::memory_order_relaxed);
  else
    sample_->flags.fetch_and(~AudioSample::kSimulateStereo,
                             std::memory_order_relaxed);
}

void AudioResource::SetResampleStep(size_t step) {
  sample_->step.store(step + 100, std::memory_order_relaxed);
}

void AudioResource::SetMaxAmplitude(float max_amplitude) {
  sample_->max_amplitude.store(max_amplitude, std::memory_order_relaxed);
}

void AudioResource::SetAmplitudeInc(float amplitude_inc) {
  sample_->amplitude_inc.store(amplitude_inc, std::memory_order_relaxed);
}

void AudioResource::SetEndCallback(base::Closure cb) {
  sample_->end_cb = cb;
}

}  // namespace eng
