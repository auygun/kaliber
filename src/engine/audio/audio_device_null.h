#ifndef ENGINE_AUDIO_AUDIO_DEVICE_NULL_H
#define ENGINE_AUDIO_AUDIO_DEVICE_NULL_H

#include "engine/audio/audio_device.h"

namespace eng {

class AudioDeviceNull final : public AudioDevice {
 public:
  AudioDeviceNull() = default;
  ~AudioDeviceNull() final = default;

  bool Initialize() final { return true; }

  void Suspend() final {}
  void Resume() final {}

  size_t GetHardwareSampleRate() final { return 0; }
};

}  // namespace eng

#endif  // ENGINE_AUDIO_AUDIO_DEVICE_NULL_H
