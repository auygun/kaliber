#ifndef ENGINE_AUDIO_AUDIO_SINK_NULL_H
#define ENGINE_AUDIO_AUDIO_SINK_NULL_H

#include "engine/audio/audio_sink.h"

namespace eng {

class AudioSinkNull final : public AudioSink {
 public:
  AudioSinkNull() = default;
  ~AudioSinkNull() final = default;

  bool Initialize() final { return true; }

  void Suspend() final {}
  void Resume() final {}

  size_t GetHardwareSampleRate() final { return 0; }
};

}  // namespace eng

#endif  // ENGINE_AUDIO_AUDIO_SINK_NULL_H
