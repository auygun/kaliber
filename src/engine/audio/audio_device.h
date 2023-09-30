#ifndef ENGINE_AUDIO_AUDIO_DEVICE_H
#define ENGINE_AUDIO_AUDIO_DEVICE_H

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
