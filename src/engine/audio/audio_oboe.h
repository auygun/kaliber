#ifndef AUDIO_OBOE_H
#define AUDIO_OBOE_H

#include <memory>

#include "../../third_party/oboe/include/oboe/AudioStream.h"
#include "../../third_party/oboe/include/oboe/AudioStreamCallback.h"
#include "audio_base.h"

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
  class StreamCallback : public oboe::AudioStreamCallback {
   public:
    StreamCallback(AudioOboe* audio);
    ~StreamCallback() override;

    oboe::DataCallbackResult onAudioReady(oboe::AudioStream* oboe_stream,
                                          void* audio_data,
                                          int32_t num_frames) override;

    void onErrorAfterClose(oboe::AudioStream* oboe_stream,
                           oboe::Result error) override;

   private:
    AudioOboe* audio_;
  };

  oboe::ManagedStream stream_;
  std::unique_ptr<StreamCallback> callback_;

  bool RestartStream();
};

}  // namespace eng

#endif  // AUDIO_OBOE_H
