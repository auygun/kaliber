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
  if (sound->is_streaming_sound()) {
    LOG << "Fata error! Streaming sound cannot be shared.";
    Engine::Get().Exit();
  }
  sound_ = sound;
}

void SoundPlayer::SetSound(std::unique_ptr<Sound> sound) {
  sound_ = std::move(sound);
}

void SoundPlayer::Play(bool loop) {
  if (sound_) {
    int step = variate_ ? Engine::Get().GetRandomGenerator().Roll(3) - 2 : 0;
    resource_->SetResampleStep(step);
    resource_->SetAmplitudeInc(0);
    resource_->SetLoop(loop);
    resource_->Play(sound_, max_amplitude_, true);
  }
}

void SoundPlayer::Resume(bool fade_in) {
  if (fade_in)
    resource_->SetAmplitudeInc(0.0001f);
  resource_->Play(sound_, fade_in ? 0 : max_amplitude_, false);
}

void SoundPlayer::Stop(bool fade_out) {
  if (fade_out)
    resource_->SetAmplitudeInc(-0.0001f);
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
