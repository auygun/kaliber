#ifndef ENGINE_AUDIO_PLAYER_H
#define ENGINE_AUDIO_PLAYER_H

#include <memory>

#include "base/closure.h"

namespace eng {

class AudioBus;

class SoundPlayer {
 public:
  SoundPlayer();
  ~SoundPlayer();

  void SetSound(std::shared_ptr<AudioBus> sound);

  void Play(bool loop, float fade_in_duration = 0);

  void Resume(float fade_in_duration = 0);

  void Stop(float fade_out_duration = 0);

  // Picks a random variation of the sound or the original sound if "variate" is
  // false. Variations are obtained by slightly up or down sampling.
  void SetVariate(bool variate);

  // Enable or disable stereo simulation effect. Disabled by default.
  void SetSimulateStereo(bool simulate);

  void SetMaxAplitude(float max_amplitude);

  // Set callback to be called once playback stops.
  void SetEndCallback(base::Closure cb);

 private:
  uint64_t resource_id_ = 0;
  std::shared_ptr<AudioBus> sound_;

  float max_amplitude_ = 1.0f;

  bool variate_ = false;

  SoundPlayer(const SoundPlayer&) = delete;
  SoundPlayer& operator=(const SoundPlayer&) = delete;
};

}  // namespace eng

#endif  // ENGINE_AUDIO_PLAYER_H
