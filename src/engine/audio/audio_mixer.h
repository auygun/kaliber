#ifndef ENGINE_AUDIO_AUDIO_MIXER_H
#define ENGINE_AUDIO_AUDIO_MIXER_H

#include <atomic>
#include <list>
#include <memory>
#include <mutex>

#include "base/closure.h"
#include "engine/audio/audio_sink.h"

namespace base {
class TaskRunner;
}

namespace eng {

class AudioBus;

// Mix and render audio with low overhead. A platform specific AudioSink
// implementation is expected to periodically call RenderAudio() in a background
// thread.
class AudioMixer : public AudioSink::Delegate {
 public:
  AudioMixer();
  ~AudioMixer();

  std::shared_ptr<void> CreateResource();

  void Play(std::shared_ptr<void> resource,
            std::shared_ptr<AudioBus> audio_bus,
            float amplitude,
            bool reset_pos);
  void Stop(std::shared_ptr<void> resource);

  void SetLoop(std::shared_ptr<void> resource, bool loop);
  void SetSimulateStereo(std::shared_ptr<void> resource, bool simulate);
  void SetResampleStep(std::shared_ptr<void> resource, size_t step);
  void SetMaxAmplitude(std::shared_ptr<void> resource, float max_amplitude);
  void SetAmplitudeInc(std::shared_ptr<void> resource, float amplitude_inc);
  void SetEndCallback(std::shared_ptr<void> resource, base::Closure cb);

  void SetEnableAudio(bool enable) { audio_enabled_ = enable; }

  void Suspend();
  void Resume();

  size_t GetHardwareSampleRate();

 private:
  enum SampleFlags { kLoop = 1, kStopped = 2, kSimulateStereo = 4 };
  static constexpr int kChannelCount = 2;

  // An audio resource that gets mixed and rendered to the audio sink.
  struct Resource {
    // Accessed by main thread only.
    bool active = false;
    base::Closure end_cb;
    base::Closure restart_cb;

    // Initialized by main thread, used by audio thread.
    std::shared_ptr<AudioBus> audio_bus;
    size_t src_index = 0;
    size_t accumulator = 0;
    float amplitude = 1.0f;

    // Write accessed by main thread, read-only accessed by audio thread.
    std::atomic<unsigned> flags{0};
    std::atomic<size_t> step{100};
    std::atomic<float> amplitude_inc{0};
    std::atomic<float> max_amplitude{1.0f};

    // Accessed by audio thread and decoder thread.
    std::atomic<bool> streaming_in_progress{false};

    ~Resource();
  };

  std::list<std::shared_ptr<Resource>> play_list_[2];
  std::mutex lock_;

  std::list<std::shared_ptr<Resource>> end_list_;

  std::shared_ptr<base::TaskRunner> main_thread_task_runner_;

  std::unique_ptr<AudioSink> audio_sink_;

  bool audio_enabled_ = true;

  // AudioSink::Delegate interface
  int GetChannelCount() final { return kChannelCount; }
  void RenderAudio(float* output_buffer, size_t num_frames) final;

  void DoStream(std::shared_ptr<Resource> sample, bool loop);

  void EndCallback(std::shared_ptr<Resource> sample);

  AudioMixer(const AudioMixer&) = delete;
  AudioMixer& operator=(const AudioMixer&) = delete;
};

}  // namespace eng

#endif  // ENGINE_AUDIO_AUDIO_MIXER_H
