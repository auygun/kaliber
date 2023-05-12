#ifndef ENGINE_AUDIO_AUDIO_OBOE_H
#define ENGINE_AUDIO_AUDIO_OBOE_H

#include <memory>

#include "third_party/oboe/include/oboe/AudioStream.h"
#include "third_party/oboe/include/oboe/AudioStreamCallback.h"

#include "engine/audio/audio_base.h"

namespace eng {

class AudioOboe : public AudioBase {
 public:
  AudioOboe();
  ~AudioOboe();

  bool Initialize();

  void Shutdown();

  void Suspend();
  void Resume();

  int GetHardwareSampleRate();

 private:
  class StreamCallback final : public oboe::AudioStreamCallback {
   public:
    StreamCallback(AudioOboe* audio);
    ~StreamCallback() final;

    oboe::DataCallbackResult onAudioReady(oboe::AudioStream* oboe_stream,
                                          void* audio_data,
                                          int32_t num_frames) final;

    void onErrorAfterClose(oboe::AudioStream* oboe_stream,
                           oboe::Result error) final;

   private:
    AudioOboe* audio_;
  };

  oboe::ManagedStream stream_;
  std::unique_ptr<StreamCallback> callback_;

  bool RestartStream();
};

}  // namespace eng

#endif  // ENGINE_AUDIO_AUDIO_OBOE_H
