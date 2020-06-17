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

  void Play(std::shared_ptr<const Sound> sound, float amplitude, bool reset_pos);

  void Stop();

  void SetLoop(bool loop);
  void SetSimulateStereo(bool simulate);
  void SetResampleStep(size_t step);
  void SetMaxAmplitude(float max_amplitude);
  void SetAmplitudeInc(float amplitude_inc);

 private:
  std::shared_ptr<void> impl_data_;

  Audio* audio_ = nullptr;

  AudioResource(const AudioResource&) = delete;
  AudioResource& operator=(const AudioResource&) = delete;
};

}  // namespace eng

#endif  // AUDIO_RESOURCE_H
