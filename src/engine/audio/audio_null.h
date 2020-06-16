#ifndef AUDIO_NULL_H
#define AUDIO_NULL_H

#include <memory>

namespace eng {

class Sound;

class AudioNull {
 public:
  AudioNull() = default;
  ~AudioNull() = default;

  bool Initialize() { return true; }

  void Shutdown() {}

  std::shared_ptr<AudioResource> CreateResource() { return nullptr; }

  void Play(std::shared_ptr<const Sound> sound,
            std::shared_ptr<void> impl_data,
            bool loop,
            size_t step) {}

  void Stop(std::shared_ptr<void> impl_data) {}
};

}  // namespace eng

#endif  // AUDIO_NULL_H
