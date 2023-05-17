#ifndef ENGINE_AUDIO_AUDIO_DRIVER_H
#define ENGINE_AUDIO_AUDIO_DRIVER_H

#include "engine/audio/audio_driver.h"

namespace eng {

class AudioDriverDelegate;

// Models an audio sink sending mixed audio to the audio driver. Audio data from
// the mixer source is delivered on a pull model using AudioDriverDelegate.
class AudioDriver {
 public:
  AudioDriver() = default;
  virtual ~AudioDriver() = default;

  virtual void SetDelegate(AudioDriverDelegate* delegate) = 0;

  virtual bool Initialize() = 0;

  virtual void Shutdown() = 0;

  virtual void Suspend() = 0;
  virtual void Resume() = 0;

  virtual int GetHardwareSampleRate() = 0;

 private:
  AudioDriver(const AudioDriver&) = delete;
  AudioDriver& operator=(const AudioDriver&) = delete;
};

}  // namespace eng

#endif  // ENGINE_AUDIO_AUDIO_DRIVER_H
