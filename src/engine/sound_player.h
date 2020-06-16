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

  void Play(bool loop, bool variate);

  void Stop();

 private:
  std::shared_ptr<AudioResource> resource_;
  std::shared_ptr<const Sound> sound_;

  SoundPlayer(const SoundPlayer&) = delete;
  SoundPlayer& operator=(const SoundPlayer&) = delete;
};

}  // namespace eng

#endif  // AUDIO_PLAYER_H
