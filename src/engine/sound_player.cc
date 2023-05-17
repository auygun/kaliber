#include "engine/sound_player.h"

#include "base/interpolation.h"
#include "base/log.h"
#include "engine/audio/audio_mixer.h"
#include "engine/engine.h"
#include "engine/sound.h"

using namespace base;

namespace eng {

SoundPlayer::SoundPlayer()
    : resource_id_(Engine::Get().GetAudioMixer()->CreateResource()) {}

SoundPlayer::~SoundPlayer() {
  Engine::Get().GetAudioMixer()->DestroyResource(resource_id_);
}

void SoundPlayer::SetSound(std::shared_ptr<Sound> sound) {
  CHECK(!sound->is_streaming_sound()) << "Streaming sound cannot be shared.";

  sound_ = sound;
}

void SoundPlayer::SetSound(std::unique_ptr<Sound> sound) {
  sound_ = std::move(sound);
}

void SoundPlayer::Play(bool loop, float fade_in_duration) {
  if (!sound_)
    return;

  int step = variate_ ? Engine::Get().GetRandomGenerator().Roll(3) - 2 : 0;
  Engine::Get().GetAudioMixer()->SetResampleStep(resource_id_, step * 12);
  Engine::Get().GetAudioMixer()->SetLoop(resource_id_, loop);
  if (fade_in_duration > 0)
    Engine::Get().GetAudioMixer()->SetAmplitudeInc(
        resource_id_, 1.0f / (sound_->sample_rate() * fade_in_duration));
  else
    Engine::Get().GetAudioMixer()->SetAmplitudeInc(resource_id_, 0);
  Engine::Get().GetAudioMixer()->Play(
      resource_id_, sound_, fade_in_duration > 0 ? 0 : max_amplitude_, true);
}

void SoundPlayer::Resume(float fade_in_duration) {
  if (!sound_)
    return;

  if (fade_in_duration > 0)
    Engine::Get().GetAudioMixer()->SetAmplitudeInc(
        resource_id_, 1.0f / (sound_->sample_rate() * fade_in_duration));
  Engine::Get().GetAudioMixer()->Play(resource_id_, sound_,
                                      fade_in_duration > 0 ? 0 : -1, false);
}

void SoundPlayer::Stop(float fade_out_duration) {
  if (!sound_)
    return;

  if (fade_out_duration > 0)
    Engine::Get().GetAudioMixer()->SetAmplitudeInc(
        resource_id_, -1.0f / (sound_->sample_rate() * fade_out_duration));
  else
    Engine::Get().GetAudioMixer()->Stop(resource_id_);
}

void SoundPlayer::SetVariate(bool variate) {
  variate_ = variate;
}

void SoundPlayer::SetSimulateStereo(bool simulate) {
  Engine::Get().GetAudioMixer()->SetSimulateStereo(resource_id_, simulate);
}

void SoundPlayer::SetMaxAplitude(float max_amplitude) {
  max_amplitude_ = max_amplitude;
  Engine::Get().GetAudioMixer()->SetMaxAmplitude(resource_id_, max_amplitude);
}

void SoundPlayer::SetEndCallback(base::Closure cb) {
  Engine::Get().GetAudioMixer()->SetEndCallback(resource_id_, cb);
}

}  // namespace eng
