#include "audio_resource.h"

#include "../../base/log.h"
#include "audio.h"

namespace eng {

AudioResource::AudioResource(std::shared_ptr<void> impl_data, Audio* audio)
    : impl_data_(impl_data), audio_(audio) {}

AudioResource::~AudioResource() {
  audio_->Stop(impl_data_);
}

void AudioResource::Play(std::shared_ptr<const Sound> sound,
                         float amplitude,
                         bool reset_pos) {
  audio_->Play(impl_data_, sound, amplitude, reset_pos);
}

void AudioResource::Stop() {
  audio_->Stop(impl_data_);
}

void AudioResource::SetLoop(bool loop) {
  audio_->SetLoop(impl_data_, loop);
}

void AudioResource::SetSimulateStereo(bool simulate) {
  audio_->SetSimulateStereo(impl_data_, simulate);
}

void AudioResource::SetResampleStep(size_t step) {
  audio_->SetResampleStep(impl_data_, step);
}

void AudioResource::SetMaxAmplitude(float max_amplitude) {
  audio_->SetMaxAmplitude(impl_data_, max_amplitude);
}

void AudioResource::SetAmplitudeInc(float amplitude_inc) {
  audio_->SetAmplitudeInc(impl_data_, amplitude_inc);
}

void AudioResource::SetEndCallback(base::Closure cb) {
  audio_->SetEndCallback(impl_data_, cb);
}

}  // namespace eng
