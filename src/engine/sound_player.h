#ifndef AUDIO_PLAYER_H
#define AUDIO_PLAYER_H

#include <memory>

namespace eng {

class AudioResource;
class Sound;

class SoundPlayer {
 public:
  SoundPlayer();
  ~SoundPlayer();

  void SetSound(std::shared_ptr<const Sound> sound);

  void Play(bool loop);

  void Resume(bool fade_in);

  void Stop(bool fade_out);

  // Picks a random variation of the sound or the original sound if "variate" is
  // false. Variations are obtained by slightly up or down sampling.
  void SetVariate(bool variate);

  // Enable or disable stereo simulation effect. Valid for mono samples only.
  // Disabled by default.
  void SetSimulateStereo(bool simulate);

  // Set aplitude. Ampitude cannot be altered during playback. Must be called
  // before play/resume.
  void SetAplitude(float amplitude);

 private:
  std::shared_ptr<AudioResource> resource_;
  std::shared_ptr<const Sound> sound_;

  float amplitude_ = 1.0f;

  SoundPlayer(const SoundPlayer&) = delete;
  SoundPlayer& operator=(const SoundPlayer&) = delete;
};

}  // namespace eng

#endif  // AUDIO_PLAYER_H
