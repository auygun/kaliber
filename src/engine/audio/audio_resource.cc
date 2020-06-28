#include "audio_resource.h"

#include "../../base/log.h"
#include "../sound.h"
#include "audio.h"
#include "audio_sample.h"

namespace eng {

AudioResource::AudioResource(Audio* audio)
    : sample_(std::make_shared<AudioSample>()), audio_(audio) {}

AudioResource::~AudioResource() {
  sample_->flags |= AudioSample::kStopped;
}

void AudioResource::Play(std::shared_ptr<Sound> sound,
                         float amplitude,
                         bool reset_pos) {
  if (sample_->active)
    return;

  if (reset_pos) {
    sample_->src_index = 0;
    sample_->accumulator = 0;
  }
  sample_->flags &= ~AudioSample::kStopped;
  sample_->sound = sound;
  sample_->amplitude = amplitude;
  sample_->active = true;

  audio_->Play(sample_);
}

void AudioResource::Stop() {
  if (!sample_->active)
    return;

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
