#ifndef AUDIO_BASE_H
#define AUDIO_BASE_H

#include <list>
#include <memory>
#include <mutex>

#include "../../base/closure.h"
#include "audio_sample.h"

namespace base {
class TaskRunner;
}

namespace eng {

class Sound;

class AudioBase {
 public:
  void Play(std::shared_ptr<AudioSample> sample);

  void SetEnableAudio(bool enable) { audio_enabled_ = enable; }

 protected:
  static constexpr int kChannelCount = 2;

  AudioBase();
  ~AudioBase();

  void RenderAudio(float* output_buffer, size_t num_frames);

 private:
  std::list<std::shared_ptr<AudioSample>> samples_[2];
  std::mutex lock_;

  base::TaskRunner* main_thread_task_runner_;

  bool audio_enabled_ = true;

  void DoStream(std::shared_ptr<AudioSample> sample, bool loop);

  void EndCallback(std::shared_ptr<AudioSample> sample);

  AudioBase(const AudioBase&) = delete;
  AudioBase& operator=(const AudioBase&) = delete;
};

}  // namespace eng

#endif  // AUDIO_BASE_H
