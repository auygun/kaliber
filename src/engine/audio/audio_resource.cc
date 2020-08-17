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
  sample_->flags |= AudioSample::kStopped;
}

void AudioResource::Play(std::shared_ptr<Sound> sound,
                         float amplitude,
                         bool reset_pos) {
  AudioSample* sample = sample_.get();

  if (sample->active) {
    if (reset_pos)
      sample->flags |= AudioSample::kStopped;

    if (sample->flags & AudioSample::kStopped) {
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
  sample->flags &= ~AudioSample::kStopped;
  sample->sound = sound;
  if (amplitude >= 0)
    sample->amplitude = amplitude;

  audio_->Play(sample_);
}

void AudioResource::Stop() {
  if (sample_->active)
    sample_->flags |= AudioSample::kStopped;
}

void AudioResource::SetLoop(bool loop) {
  if (loop)
    sample_->flags |= AudioSample::kLoop;
  else
    sample_->flags &= ~AudioSample::kLoop;
}

void AudioResource::SetSimulateStereo(bool simulate) {
  if (simulate)
    sample_->flags |= AudioSample::kSimulateStereo;
  else
    sample_->flags &= ~AudioSample::kSimulateStereo;
}

void AudioResource::SetResampleStep(size_t step) {
  sample_->step = step + 10;
}

void AudioResource::SetMaxAmplitude(float max_amplitude) {
  sample_->max_amplitude = max_amplitude;
}

void AudioResource::SetAmplitudeInc(float amplitude_inc) {
  sample_->amplitude_inc = amplitude_inc;
}

void AudioResource::SetEndCallback(base::Closure cb) {
  sample_->end_cb = cb;
}

}  // namespace eng
