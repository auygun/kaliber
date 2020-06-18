#ifndef AUDIO_BASE_H
#define AUDIO_BASE_H

#include <memory>
#include <mutex>
#include <list>

namespace eng {

class Sound;

class AudioBase {
 public:
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

 protected:
  enum SampleFlags { kLoop = 1, kStopped = 2, kSimulateStereo = 4 };

  static constexpr int kChannelCount = 2;

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

  std::list<std::shared_ptr<Sample>> samples_[2];
  std::mutex mutex_;

  AudioBase();
  ~AudioBase();

  void RenderAudio(float *output_buffer, size_t num_frames);
};

}  // namespace eng

#endif  // AUDIO_BASE_H
