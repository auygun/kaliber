#ifndef AUDIO_PLAYER_H
#define AUDIO_PLAYER_H

#include <memory>

#include "../base/closure.h"

namespace eng {

class AudioResource;
class Sound;

class SoundPlayer {
 public:
  SoundPlayer();
  ~SoundPlayer();

  void SetSound(std::shared_ptr<Sound> sound);
  void SetSound(std::unique_ptr<Sound> sound);

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
  std::unique_ptr<AudioResource> resource_;
  std::shared_ptr<Sound> sound_;

  float max_amplitude_ = 1.0f;

  bool variate_ = false;

  SoundPlayer(const SoundPlayer&) = delete;
  SoundPlayer& operator=(const SoundPlayer&) = delete;
};

}  // namespace eng

#endif  // AUDIO_PLAYER_H
