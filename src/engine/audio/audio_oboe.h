#ifndef AUDIO_OBOE_H
#define AUDIO_OBOE_H

#include <memory>
#include <mutex>
#include <list>

#include "../../third_party/oboe/include/oboe/AudioStream.h"
#include "../../third_party/oboe/include/oboe/AudioStreamCallback.h"

namespace eng {

class Sound;

class AudioOboe {
 public:
  AudioOboe();
  ~AudioOboe();

  bool Initialize();

  void Shutdown();

  void Play(std::shared_ptr<const Sound> sound, bool loop);

 private:
  enum SampleFlags { kLoop = 1 };

  struct Sample {
    std::shared_ptr<const Sound> sound;
    size_t ind;
    float step;
    unsigned flags_;
  };

  class StreamCallback : public oboe::AudioStreamCallback {
   public:
    StreamCallback(AudioOboe* audio);
    ~StreamCallback() override;

    oboe::DataCallbackResult onAudioReady(oboe::AudioStream *oboe_stream,
                                          void *audio_data,
                                          int32_t num_frames) override;

    void onErrorAfterClose(oboe::AudioStream *oboe_stream,
                           oboe::Result error) override;

   private:
    AudioOboe* audio_;
  };

  oboe::ManagedStream stream_;
  std::unique_ptr<StreamCallback> callback_;

  std::list<Sample> samples_[2];
  std::mutex mutex_;

  void RenderAudio(float *output_buffer, int32_t num_frames);
};

}  // namespace eng

#endif  // AUDIO_OBOE_H
