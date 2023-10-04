#ifndef ENGINE_AUDIO_AUDIO_DEVICE_WASAPI_H
#define ENGINE_AUDIO_AUDIO_DEVICE_WASAPI_H

#include <thread>

#include <AudioClient.h>
#include <MMDeviceAPI.h>

#include "engine/audio/audio_device.h"

namespace eng {

class AudioDeviceWASAPI final : public AudioDevice {
 public:
  AudioDeviceWASAPI(AudioDevice::Delegate* delegate);
  ~AudioDeviceWASAPI() final;

  bool Initialize() final;

  void Suspend() final;
  void Resume() final;

  size_t GetHardwareSampleRate() final;

 private:
  IMMDevice* device_ = nullptr;
  IAudioClient* audio_client_ = nullptr;
  IAudioRenderClient* render_client_ = nullptr;
  IMMDeviceEnumerator* device_enumerator_ = nullptr;

  HANDLE shutdown_event_ = nullptr;
  HANDLE ready_event_ = nullptr;

  std::thread audio_thread_;

  UINT32 buffer_size_ = 0;
  size_t sample_rate_ = 0;

  AudioDevice::Delegate* delegate_ = nullptr;

  void StartAudioThread();
  void TerminateAudioThread();

  void AudioThreadMain();
};

}  // namespace eng

#endif  // ENGINE_AUDIO_AUDIO_DEVICE_WASAPI_H
