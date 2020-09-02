#ifndef AUDIO_SAMPLE_H
#define AUDIO_SAMPLE_H

#include <atomic>
#include <memory>

#include "../../base/closure.h"

namespace eng {

class Sound;

struct AudioSample {
  enum SampleFlags { kLoop = 1, kStopped = 2, kSimulateStereo = 4 };

  // Accessed by main thread only.
  bool active = false;
  base::Closure end_cb;

  // Accessed by audio thread only.
  bool marked_for_removal = false;

  // Initialized by main thread, used by audio thread.
  std::shared_ptr<Sound> sound;
  size_t src_index = 0;
  size_t accumulator = 0;
  float amplitude = 1.0f;

  // Write accessed by main thread, read-only accessed by audio thread.
  std::atomic<unsigned> flags{0};
  std::atomic<size_t> step{10};
  std::atomic<float> amplitude_inc{0};
  std::atomic<float> max_amplitude{1.0f};

  // Accessed by audio thread and decoder thread.
  std::atomic<bool> streaming_in_progress{false};
};

}  // namespace eng

#endif  // AUDIO_SAMPLE_H
