#ifndef AUDIO_NULL_H
#define AUDIO_NULL_H

#include <memory>

namespace eng {

class AudioResource;
class Sound;

class AudioNull {
 public:
  AudioNull() = default;
  ~AudioNull() = default;

  bool Initialize() { return true; }

  void Shutdown() {}

  std::shared_ptr<AudioResource> CreateResource() { return nullptr; }

  void Play(std::shared_ptr<void> impl_data,
            std::shared_ptr<const Sound> sound,
            float amplitude,
            bool reset_pos) {}

  void Stop(std::shared_ptr<void> impl_data) {}

  void SetLoop(std::shared_ptr<void> impl_data, bool loop) {}
  void SetSimulateStereo(std::shared_ptr<void> impl_data, bool simulate) {}
  void SetResampleStep(std::shared_ptr<void> impl_data, size_t step) {}
  void SetMaxAmplitude(std::shared_ptr<void> impl_data, float max_amplitude) {}
  void SetAmplitudeInc(std::shared_ptr<void> impl_data, float amplitude_inc) {}

  size_t GetSampleRate() { return 0; }
};

}  // namespace eng

#endif  // AUDIO_NULL_H
