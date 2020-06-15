#include "audio_player.h"

#include "../base/interpolation.h"
#include "audio/audio_resource.h"
#include "engine.h"

using namespace base;

namespace eng {

AudioPlayer::AudioPlayer() : resource_(Engine::Get().CreateAudioResource()) {}

AudioPlayer::~AudioPlayer() = default;

void AudioPlayer::SetSound(std::shared_ptr<const Sound> sound) {
  sound_ = sound;
}

void AudioPlayer::Play(bool loop, bool variate) {
  if (resource_) {
    int step = variate
               ? Lerp(9, 11, Engine::Get().GetRandomGenerator().GetFloat())
               : 1;
    resource_->Play(sound_, loop, step);
  }
}

}  // namespace eng
