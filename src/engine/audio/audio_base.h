#ifndef AUDIO_BASE_H
#define AUDIO_BASE_H

#include <list>
#include <memory>
#include <mutex>

#include "../../base/closure.h"
#include "../../base/task_runner.h"
#include "../../base/worker.h"
#include "audio_sample.h"

namespace eng {

class Sound;

class AudioBase {
 public:
  void Play(std::shared_ptr<AudioSample> impl_data);

  void Update();

 protected:
  static constexpr int kChannelCount = 2;

  std::list<std::shared_ptr<AudioSample>> samples_[2];
  std::mutex mutex_;

  base::Worker worker_{1};

  base::TaskRunner task_runner_;

  AudioBase();
  ~AudioBase();

  void RenderAudio(float* output_buffer, size_t num_frames);
};

}  // namespace eng

#endif  // AUDIO_BASE_H
