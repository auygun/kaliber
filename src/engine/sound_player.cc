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
  sound_ = Engine::Get().GetAudioBus(asset_name);
}

void SoundPlayer::SetSound(std::shared_ptr<AudioBus> sound) {
  sound_ = sound;
}

void SoundPlayer::Play(bool loop, float fade_in_duration) {
  if (!sound_)
    return;

  int step = variate_ ? Engine::Get().GetRandomGenerator().Roll(3) - 2 : 0;
  input_->SetResampleStep(step * 12);
  input_->SetLoop(loop);
  if (fade_in_duration > 0)
    input_->SetAmplitudeInc(1.0f / (sound_->sample_rate() * fade_in_duration));
  else
    input_->SetAmplitudeInc(0);
  input_->Play(Engine::Get().GetAudioMixer(), sound_,
               fade_in_duration > 0 ? 0 : max_amplitude_, true);
}

void SoundPlayer::Resume(float fade_in_duration) {
  if (!sound_)
    return;

  if (fade_in_duration > 0)
    input_->SetAmplitudeInc(1.0f / (sound_->sample_rate() * fade_in_duration));
  input_->Play(Engine::Get().GetAudioMixer(), sound_,
               fade_in_duration > 0 ? 0 : -1, false);
}

void SoundPlayer::Stop(float fade_out_duration) {
  if (!sound_)
    return;

  if (fade_out_duration > 0)
    input_->SetAmplitudeInc(-1.0f /
                            (sound_->sample_rate() * fade_out_duration));
  else
    input_->Stop();
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
