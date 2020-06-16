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

void SoundPlayer::Play(bool loop, bool variate) {
  if (resource_) {
    int step = variate
               ? Lerp(9, 11, Engine::Get().GetRandomGenerator().GetFloat())
               : 1;
    resource_->Play(sound_, loop, step);
  }
}

void SoundPlayer::Stop() {
  if (resource_)
    resource_->Stop();
}

}  // namespace eng
