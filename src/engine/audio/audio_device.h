#ifndef ENGINE_AUDIO_AUDIO_DEVICE_H
#define ENGINE_AUDIO_AUDIO_DEVICE_H

#include <list>
#include <string>

namespace eng {

// Models an audio device sending mixed audio to the audio driver. Audio data
// from the mixer source is delivered on a pull model using Delegate.
class AudioDevice {
 public:
  class Delegate {
   public:
    Delegate() = default;
    virtual ~Delegate() = default;

    virtual int GetChannelCount() = 0;

    virtual void RenderAudio(float* output_buffer, size_t num_frames) = 0;
  };

  struct DeviceName {
    std::string device_name;  // Friendly name of the device.
    std::string unique_id;    // Unique identifier for the device.
  };

  typedef std::list<DeviceName> DeviceNames;

  AudioDevice() = default;
  virtual ~AudioDevice() = default;

  virtual bool Initialize() = 0;

  virtual void Suspend() = 0;
  virtual void Resume() = 0;

  virtual size_t GetHardwareSampleRate() = 0;

 private:
  AudioDevice(const AudioDevice&) = delete;
  AudioDevice& operator=(const AudioDevice&) = delete;
};

}  // namespace eng

#endif  // ENGINE_AUDIO_AUDIO_DEVICE_H
