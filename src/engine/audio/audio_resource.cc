#include "audio_resource.h"

#include "audio.h"

namespace eng {

AudioResource::AudioResource(std::shared_ptr<void> impl_data, Audio* audio)
    : impl_data_(impl_data), audio_(audio) {}

AudioResource::~AudioResource() = default;

void AudioResource::Play(std::shared_ptr<const Sound> sound,
                         bool loop,
                         int step) {
  audio_->Play(sound, impl_data_, loop, step);
}

}  // namespace eng
