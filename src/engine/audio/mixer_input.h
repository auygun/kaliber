#ifndef ENGINE_AUDIO_MIXER_INPUT_H
#define ENGINE_AUDIO_MIXER_INPUT_H

#include <atomic>
#include <memory>

#include "base/closure.h"

namespace eng {

class AudioBus;
class AudioMixer;

// An audio input stream that gets mixed and rendered to the audio sink. Handles
// playback and volume control.
struct MixerInput : public std::enable_shared_from_this<MixerInput> {
  enum Flags { kLoop = 1, kStopped = 2, kSimulateStereo = 4 };

  ~MixerInput();

  static std::shared_ptr<MixerInput> Create();

  void Play(AudioMixer* mixer,
            std::shared_ptr<AudioBus> bus,
            float amp,
            bool restart);
  void Stop();

  void SetLoop(bool loop);
  void SetSimulateStereo(bool simulate);
  void SetResampleStep(size_t value);
  void SetMaxAmplitude(float value);
  void SetAmplitudeInc(float value);
  void SetEndCallback(base::Closure cb);

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

 private:
  MixerInput();
};

}  // namespace eng

#endif  // ENGINE_AUDIO_MIXER_INPUT_H
