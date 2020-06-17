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

  void Play(std::shared_ptr<void> impl_data,
            std::shared_ptr<const Sound> sound,
            float amplitude,
            bool reset_pos);

  void Stop(std::shared_ptr<void> impl_data);

  void SetLoop(std::shared_ptr<void> impl_data, bool loop);
  void SetSimulateStereo(std::shared_ptr<void> impl_data, bool simulate);
  void SetResampleStep(std::shared_ptr<void> impl_data, size_t step);
  void SetMaxAmplitude(std::shared_ptr<void> impl_data, float max_amplitude);
  void SetAmplitudeInc(std::shared_ptr<void> impl_data, float amplitude_inc);

  size_t GetSampleRate();

 private:
  enum SampleFlags { kLoop = 1, kStopped = 2, kSimulateStereo = 4 };

  struct Sample {
    // Read-only accessed by the audio thread.
    std::shared_ptr<const Sound> sound;
    unsigned flags = 0;
    size_t step = 10;
    float amplitude_inc = 0;
    float max_amplitude = 1.0f;

    // Write accessed by the audio thread.
    size_t src_index = 0;
    size_t accumulator = 0;
    float amplitude = 1.0f;
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
