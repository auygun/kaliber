#include "engine/sound_player.h"

#include "base/interpolation.h"
#include "base/log.h"
#include "engine/audio/audio_bus.h"
#include "engine/audio/audio_mixer.h"
#include "engine/engine.h"

using namespace base;

namespace eng {

SoundPlayer::SoundPlayer()
    : resource_(Engine::Get().GetAudioMixer()->CreateResource()) {}

SoundPlayer::~SoundPlayer() {
  Engine::Get().GetAudioMixer()->Stop(resource_);
}

void SoundPlayer::SetSound(const std::string& asset_name) {
  sound_ = Engine::Get().GetAudioBus(asset_name);
}

void SoundPlayer::SetSound(std::shared_ptr<AudioBus> sound) {
  sound_ = sound;
}

void SoundPlayer::Play(bool loop, float fade_in_duration) {
  if (!sound_)
    return;

  int step = variate_ ? Engine::Get().GetRandomGenerator().Roll(3) - 2 : 0;
  Engine::Get().GetAudioMixer()->SetResampleStep(resource_, step * 12);
  Engine::Get().GetAudioMixer()->SetLoop(resource_, loop);
  if (fade_in_duration > 0)
    Engine::Get().GetAudioMixer()->SetAmplitudeInc(
        resource_, 1.0f / (sound_->sample_rate() * fade_in_duration));
  else
    Engine::Get().GetAudioMixer()->SetAmplitudeInc(resource_, 0);
  Engine::Get().GetAudioMixer()->Play(
      resource_, sound_, fade_in_duration > 0 ? 0 : max_amplitude_, true);
}

void SoundPlayer::Resume(float fade_in_duration) {
  if (!sound_)
    return;

  if (fade_in_duration > 0)
    Engine::Get().GetAudioMixer()->SetAmplitudeInc(
        resource_, 1.0f / (sound_->sample_rate() * fade_in_duration));
  Engine::Get().GetAudioMixer()->Play(resource_, sound_,
                                      fade_in_duration > 0 ? 0 : -1, false);
}

void SoundPlayer::Stop(float fade_out_duration) {
  if (!sound_)
    return;

  if (fade_out_duration > 0)
    Engine::Get().GetAudioMixer()->SetAmplitudeInc(
        resource_, -1.0f / (sound_->sample_rate() * fade_out_duration));
  else
    Engine::Get().GetAudioMixer()->Stop(resource_);
}

void SoundPlayer::SetVariate(bool variate) {
  variate_ = variate;
}

void SoundPlayer::SetSimulateStereo(bool simulate) {
  Engine::Get().GetAudioMixer()->SetSimulateStereo(resource_, simulate);
}

void SoundPlayer::SetMaxAplitude(float max_amplitude) {
  max_amplitude_ = max_amplitude;
  Engine::Get().GetAudioMixer()->SetMaxAmplitude(resource_, max_amplitude);
}

void SoundPlayer::SetEndCallback(base::Closure cb) {
  Engine::Get().GetAudioMixer()->SetEndCallback(resource_, cb);
}

}  // namespace eng
