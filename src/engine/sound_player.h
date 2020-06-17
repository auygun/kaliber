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

  void Resume();

  void Stop(bool fade_out);

  void SetVariate(bool variate) { variate_ = variate; }
  void SetSimulateStereo(bool simulate) { simulate_stereo_ = simulate; }
  void SetAplitude(float amplitude) { amplitude_ = amplitude; }

 private:
  std::shared_ptr<AudioResource> resource_;
  std::shared_ptr<const Sound> sound_;

  bool variate_ = false;
  bool simulate_stereo_ = false;  // For mono samples only.
  float amplitude_ = 1.0f;

  SoundPlayer(const SoundPlayer&) = delete;
  SoundPlayer& operator=(const SoundPlayer&) = delete;
};

}  // namespace eng

#endif  // AUDIO_PLAYER_H
