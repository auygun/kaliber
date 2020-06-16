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
                         bool loop,
                         size_t step) {
  audio_->Play(sound, impl_data_, loop, step);
}

void AudioResource::Stop() {
  audio_->Stop(impl_data_);
}

}  // namespace eng
