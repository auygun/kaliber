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

  void Pause();

  void Resume();

  void Stop();

  void SetVariate(bool variate) { variate_ = variate; }
  void SetSimulateStereo(bool simulate) { simulate_stereo_ = simulate; }

 private:
  std::shared_ptr<AudioResource> resource_;
  std::shared_ptr<const Sound> sound_;

  bool variate_ = false;
  bool simulate_stereo_ = false;  // For mono samples only.

  SoundPlayer(const SoundPlayer&) = delete;
  SoundPlayer& operator=(const SoundPlayer&) = delete;
};

}  // namespace eng

#endif  // AUDIO_PLAYER_H
