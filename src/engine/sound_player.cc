#include "engine/sound_player.h"

#include "base/interpolation.h"
#include "base/log.h"
#include "engine/audio/audio_bus.h"
#include "engine/audio/audio_mixer.h"
#include "engine/audio/mixer_input.h"
#include "engine/engine.h"

using namespace base;

namespace eng {

SoundPlayer::SoundPlayer() : input_{MixerInput::Create()} {}

SoundPlayer::~SoundPlayer() {
  input_->Stop();
}

void SoundPlayer::SetSound(const std::string& asset_name) {
  input_->SetAudioBus(Engine::Get().GetAudioBus(asset_name));
}

void SoundPlayer::SetSound(std::shared_ptr<AudioBus> sound) {
  input_->SetAudioBus(sound);
}

void SoundPlayer::Play(bool loop, float fade_in_duration) {
  if (!input_->IsValid())
    return;

  int step = variate_ ? Engine::Get().GetRandomGenerator().Roll(3) - 2 : 0;
  input_->SetResampleStep(step * 12);
  input_->SetLoop(loop);
  if (fade_in_duration > 0) {
    input_->SetAmplitude(0);
    input_->SetAmplitudeInc(
        1.0f / (input_->GetAudioBus()->sample_rate() * fade_in_duration));
  } else {
    input_->SetAmplitude(max_amplitude_);
    input_->SetAmplitudeInc(0);
  }
  input_->Play(Engine::Get().GetAudioMixer(), true);
}

void SoundPlayer::Resume(float fade_in_duration) {
  if (!input_->IsValid())
    return;

  if (fade_in_duration > 0) {
    input_->SetAmplitude(0);
    input_->SetAmplitudeInc(
        1.0f / (input_->GetAudioBus()->sample_rate() * fade_in_duration));
  }
  input_->Play(Engine::Get().GetAudioMixer(), false);
}

void SoundPlayer::Stop(float fade_out_duration) {
  if (!input_->IsValid())
    return;

  if (fade_out_duration > 0)
    input_->SetAmplitudeInc(
        -1.0f / (input_->GetAudioBus()->sample_rate() * fade_out_duration));
  else
    input_->Stop();
}

bool SoundPlayer::IsPlaying() const {
  return input_->IsPlaying();
}

void SoundPlayer::SetVariate(bool variate) {
  variate_ = variate;
}

void SoundPlayer::SetSimulateStereo(bool simulate) {
  input_->SetSimulateStereo(simulate);
}

void SoundPlayer::SetMaxAmplitude(float max_amplitude) {
  max_amplitude_ = max_amplitude;
  input_->SetMaxAmplitude(max_amplitude);
}

void SoundPlayer::SetEndCallback(base::Closure cb) {
  input_->SetEndCallback(cb);
}

}  // namespace eng
