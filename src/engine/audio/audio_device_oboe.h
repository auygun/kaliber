#ifndef ENGINE_AUDIO_AUDIO_DEVICE_OBOE_H
#define ENGINE_AUDIO_AUDIO_DEVICE_OBOE_H

#include <memory>

#include "third_party/oboe/include/oboe/AudioStream.h"
#include "third_party/oboe/include/oboe/AudioStreamCallback.h"

#include "engine/audio/audio_device.h"

namespace eng {

class AudioDeviceOboe final : public AudioDevice {
 public:
  AudioDeviceOboe(AudioDevice::Delegate* delegate);
  ~AudioDeviceOboe() final;

  bool Initialize() final;

  void Suspend() final;
  void Resume() final;

  size_t GetHardwareSampleRate() final;

 private:
  class StreamCallback final : public oboe::AudioStreamCallback {
   public:
    StreamCallback(AudioDeviceOboe* audio);
    ~StreamCallback() final;

    oboe::DataCallbackResult onAudioReady(oboe::AudioStream* oboe_stream,
                                          void* audio_data,
                                          int32_t num_frames) final;

    void onErrorAfterClose(oboe::AudioStream* oboe_stream,
                           oboe::Result error) final;

   private:
    AudioDeviceOboe* audio_device_;
  };

  oboe::ManagedStream stream_;
  std::unique_ptr<StreamCallback> callback_;

  AudioDevice::Delegate* delegate_ = nullptr;

  bool RestartStream();
};

}  // namespace eng

#endif  // ENGINE_AUDIO_AUDIO_DEVICE_OBOE_H
