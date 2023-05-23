#ifndef ENGINE_AUDIO_AUDIO_SINK_DELEGATE_H
#define ENGINE_AUDIO_AUDIO_SINK_DELEGATE_H

#include <stddef.h>

namespace eng {

class AudioSinkDelegate {
 public:
  AudioSinkDelegate() = default;
  virtual ~AudioSinkDelegate() = default;

  virtual int GetChannelCount() = 0;

  virtual void RenderAudio(float* output_buffer, size_t num_frames) = 0;
};

}  // namespace eng

#endif  // ENGINE_AUDIO_AUDIO_SINK_DELEGATE_H
