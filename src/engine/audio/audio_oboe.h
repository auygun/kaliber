#ifndef AUDIO_OBOE_H
#define AUDIO_OBOE_H

#include <memory>
#include <mutex>
#include <list>

#include "../../third_party/oboe/include/oboe/AudioStream.h"
#include "../../third_party/oboe/include/oboe/AudioStreamCallback.h"

namespace eng {

class AudioResource;
class Sound;

class AudioOboe {
 public:
  AudioOboe();
  ~AudioOboe();

  bool Initialize();

  void Shutdown();

  std::shared_ptr<AudioResource> CreateResource();

  void Play(std::shared_ptr<const Sound> sound,
            std::shared_ptr<void> impl_data,
            bool loop,
            size_t step,
            bool simulate_stereo);

  void Pause(std::shared_ptr<void> impl_data);

  void Resume(std::shared_ptr<void> impl_data);

  void Stop(std::shared_ptr<void> impl_data);

  size_t GetSampleRate();

 private:
  enum SampleFlags { kLoop = 1, kPaused = 2, kStopped = 4, kSimulateStereo = 8 };

  struct Sample {
    std::shared_ptr<const Sound> sound;
    size_t src_index = 0;
    size_t step = 0;
    size_t accumulator = 0;
    unsigned flags = 0;
    bool active = false;
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

  std::list<std::shared_ptr<Sample>> samples_[2];
  std::mutex mutex_;

  void RenderAudio(float *output_buffer, int32_t num_frames);
};

}  // namespace eng

#endif  // AUDIO_OBOE_H
