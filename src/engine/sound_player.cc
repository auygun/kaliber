#include "sound_player.h"

#include "../base/interpolation.h"
#include "../base/log.h"
#include "audio/audio_resource.h"
#include "engine.h"
#include "sound.h"

using namespace base;

namespace eng {

SoundPlayer::SoundPlayer() : resource_(Engine::Get().CreateAudioResource()) {}

SoundPlayer::~SoundPlayer() = default;

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
  resource_->SetResampleStep(step * 12);
  resource_->SetLoop(loop);
  if (fade_in_duration > 0)
    resource_->SetAmplitudeInc(1.0f /
                               (sound_->sample_rate() * fade_in_duration));
  else
    resource_->SetAmplitudeInc(0);
  resource_->Play(sound_, fade_in_duration > 0 ? 0 : max_amplitude_, true);
}

void SoundPlayer::Resume(float fade_in_duration) {
  if (!sound_)
    return;

  if (fade_in_duration > 0)
    resource_->SetAmplitudeInc(1.0f /
                               (sound_->sample_rate() * fade_in_duration));
  resource_->Play(sound_, fade_in_duration > 0 ? 0 : -1, false);
}

void SoundPlayer::Stop(float fade_out_duration) {
  if (!sound_)
    return;

  if (fade_out_duration > 0)
    resource_->SetAmplitudeInc(-1.0f /
                               (sound_->sample_rate() * fade_out_duration));
  else
    resource_->Stop();
}

void SoundPlayer::SetVariate(bool variate) {
  variate_ = variate;
}

void SoundPlayer::SetSimulateStereo(bool simulate) {
  resource_->SetSimulateStereo(simulate);
}

void SoundPlayer::SetMaxAplitude(float max_amplitude) {
  max_amplitude_ = max_amplitude;
  resource_->SetMaxAmplitude(max_amplitude);
}

void SoundPlayer::SetEndCallback(base::Closure cb) {
  resource_->SetEndCallback(cb);
}

}  // namespace eng
