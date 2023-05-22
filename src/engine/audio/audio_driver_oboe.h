#ifndef ENGINE_AUDIO_AUDIO_DRIVER_OBOE_H
#define ENGINE_AUDIO_AUDIO_DRIVER_OBOE_H

#include <memory>

#include "third_party/oboe/include/oboe/AudioStream.h"
#include "third_party/oboe/include/oboe/AudioStreamCallback.h"

#include "engine/audio/audio_driver.h"

namespace eng {

class AudioDriverDelegate;

class AudioDriverOboe final : public AudioDriver {
 public:
  AudioDriverOboe(AudioDriverDelegate* delegate);
  ~AudioDriverOboe() final;

  bool Initialize() final;

  void Suspend() final;
  void Resume() final;

  int GetHardwareSampleRate() final;

 private:
  class StreamCallback final : public oboe::AudioStreamCallback {
   public:
    StreamCallback(AudioDriverOboe* audio);
    ~StreamCallback() final;

    oboe::DataCallbackResult onAudioReady(oboe::AudioStream* oboe_stream,
                                          void* audio_data,
                                          int32_t num_frames) final;

    void onErrorAfterClose(oboe::AudioStream* oboe_stream,
                           oboe::Result error) final;

   private:
    AudioDriverOboe* driver_;
  };

  oboe::ManagedStream stream_;
  std::unique_ptr<StreamCallback> callback_;

  AudioDriverDelegate* delegate_ = nullptr;

  bool RestartStream();
};

}  // namespace eng

#endif  // ENGINE_AUDIO_AUDIO_DRIVER_OBOE_H
