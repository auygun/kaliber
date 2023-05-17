#ifndef ENGINE_AUDIO_AUDIO_DRIVER_DELEGATE_H
#define ENGINE_AUDIO_AUDIO_DRIVER_DELEGATE_H

#include <stddef.h>

namespace eng {

class AudioDriverDelegate {
 public:
  AudioDriverDelegate() = default;
  virtual ~AudioDriverDelegate() = default;

  virtual void RenderAudio(float* output_buffer, size_t num_frames) = 0;
};

}  // namespace eng

#endif  // ENGINE_AUDIO_AUDIO_DRIVER_DELEGATE_H
