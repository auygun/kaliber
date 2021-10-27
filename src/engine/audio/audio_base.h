#ifndef ENGINE_AUDIO_AUDIO_BASE_H
#define ENGINE_AUDIO_AUDIO_BASE_H

#include <list>
#include <memory>
#include <mutex>
#include <unordered_map>

#include "base/closure.h"

namespace base {
class TaskRunner;
}

namespace eng {

class Sound;

// Class representing the end-point for rendered audio. A platform specific
// implementation is expected to periodically call RenderAudio() in a background
// thread.
class AudioBase {
 public:
  uint64_t CreateResource();
  void DestroyResource(uint64_t resource_id);

  void Play(uint64_t resource_id,
            std::shared_ptr<Sound> sound,
            float amplitude,
            bool reset_pos);
  void Stop(uint64_t resource_id);

  void SetLoop(uint64_t resource_id, bool loop);
  void SetSimulateStereo(uint64_t resource_id, bool simulate);
  void SetResampleStep(uint64_t resource_id, size_t step);
  void SetMaxAmplitude(uint64_t resource_id, float max_amplitude);
  void SetAmplitudeInc(uint64_t resource_id, float amplitude_inc);
  void SetEndCallback(uint64_t resource_id, base::Closure cb);

  void SetEnableAudio(bool enable) { audio_enabled_ = enable; }

 protected:
  static constexpr int kChannelCount = 2;

  AudioBase();
  ~AudioBase();

  void RenderAudio(float* output_buffer, size_t num_frames);

 private:
  enum SampleFlags { kLoop = 1, kStopped = 2, kSimulateStereo = 4 };

  struct Resource {
    // Accessed by main thread only.
    bool active = false;
    base::Closure end_cb;

    // Initialized by main thread, used by audio thread.
    std::shared_ptr<Sound> sound;
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
  };

  std::unordered_map<uint64_t, std::shared_ptr<Resource>> resources_;
  uint64_t last_resource_id_ = 0;

  std::list<std::shared_ptr<Resource>> play_list_[2];
  std::mutex lock_;

  std::list<std::shared_ptr<Resource>> end_list_;

  base::TaskRunner* main_thread_task_runner_;

  bool audio_enabled_ = true;

  void DoStream(std::shared_ptr<Resource> sample, bool loop);

  void EndCallback(std::shared_ptr<Resource> sample);

  AudioBase(const AudioBase&) = delete;
  AudioBase& operator=(const AudioBase&) = delete;
};

}  // namespace eng

#endif  // ENGINE_AUDIO_AUDIO_BASE_H
