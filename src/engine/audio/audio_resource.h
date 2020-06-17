#ifndef AUDIO_RESOURCE_H
#define AUDIO_RESOURCE_H

#include <memory>

#include "audio_forward.h"

namespace eng {

class Sound;

class AudioResource {
 public:
  AudioResource(std::shared_ptr<void> impl_data,
                Audio* audio);
  ~AudioResource();

  void Play(std::shared_ptr<const Sound> sound,
            bool loop,
            size_t step,
            bool simulate_stereo,
            float amplitude);

  void Pause();

  void Resume();

  void Stop();

 private:
  std::shared_ptr<void> impl_data_;

  Audio* audio_ = nullptr;

  AudioResource(const AudioResource&) = delete;
  AudioResource& operator=(const AudioResource&) = delete;
};

}  // namespace eng

#endif  // AUDIO_RESOURCE_H
