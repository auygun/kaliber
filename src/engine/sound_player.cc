#include "sound_player.h"

#include "../base/interpolation.h"
#include "audio/audio_resource.h"
#include "engine.h"

using namespace base;

namespace eng {

SoundPlayer::SoundPlayer() : resource_(Engine::Get().CreateAudioResource()) {}

SoundPlayer::~SoundPlayer() = default;

void SoundPlayer::SetSound(std::shared_ptr<const Sound> sound) {
  sound_ = sound;
}

void SoundPlayer::Play(bool loop) {
  if (!resource_)
    return;
  resource_->SetAmplitudeInc(0);
  resource_->SetLoop(loop);
  resource_->Play(sound_, amplitude_, true);
}

void SoundPlayer::Resume(bool fade_in) {
  if (!resource_)
    return;
  if (fade_in) {
    resource_->SetAmplitudeInc(0.0001f);
    resource_->SetMaxAmplitude(amplitude_);
  }
  resource_->Play(sound_, fade_in ? 0 : amplitude_, false);
}

void SoundPlayer::Stop(bool fade_out) {
  if (!resource_)
    return;
  if (fade_out)
    resource_->SetAmplitudeInc(-0.0001f);
  else
    resource_->Stop();
}

void SoundPlayer::SetVariate(bool variate) {
  if (!resource_)
    return;
  int step = variate
             ? Engine::Get().GetRandomGenerator().Roll(3) - 2
             : 0;
  resource_->SetResampleStep(step);
}

void SoundPlayer::SetSimulateStereo(bool simulate) {
  if (!resource_)
    return;
  resource_->SetSimulateStereo(simulate);
}

void SoundPlayer::SetAplitude(float amplitude) {
  amplitude_ = amplitude;
}

}  // namespace eng
