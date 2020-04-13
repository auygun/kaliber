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

  void Play(bool loop);

  void Resume(bool fade_in);

  void Stop(bool fade_out);

  // Picks a random variation of the sound or the original sound if "variate" is
  // false. Variations are obtained by slightly up or down sampling.
  void SetVariate(bool variate);

  // Enable or disable stereo simulation effect. Valid for mono samples only.
  // Disabled by default.
  void SetSimulateStereo(bool simulate);

  void SetMaxAplitude(float max_amplitude);

  // Set callback to be called once playback stops.
  void SetEndCallback(base::Closure cb);

 private:
  std::shared_ptr<AudioResource> resource_;
  std::shared_ptr<Sound> sound_;

  float max_amplitude_ = 1.0f;

  SoundPlayer(const SoundPlayer&) = delete;
  SoundPlayer& operator=(const SoundPlayer&) = delete;
};

}  // namespace eng

#endif  // AUDIO_PLAYER_H
