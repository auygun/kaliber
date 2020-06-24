#ifndef AUDIO_SAMPLE_H
#define AUDIO_SAMPLE_H

#include <memory>

#include "../../base/closure.h"

namespace eng {

class Sound;

struct AudioSample {
  enum SampleFlags { kLoop = 1, kStopped = 2, kSimulateStereo = 4 };

  // Read-only accessed by the audio thread.
  std::shared_ptr<Sound> sound;
  unsigned flags = 0;
  size_t step = 10;
  float amplitude_inc = 0;
  float max_amplitude = 1.0f;
  base::Closure end_cb;

  // Write accessed by the audio thread.
  size_t src_index = 0;
  size_t accumulator = 0;
  float amplitude = 1.0f;
  bool active = false;
};

}  // namespace eng

#endif  // AUDIO_SAMPLE_H
