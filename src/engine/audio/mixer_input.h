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

  // Set AudioBus for playback. If this MixerInput is already streaming, keeps
  // it as pending and sets once playback ends.
  void SetAudioBus(std::shared_ptr<AudioBus> bus);

  // Starts playback. Remembers last stream position. Resets the stream position
  // if restart is true.
  void Play(AudioMixer* mixer, bool restart);

  // Stops playback. Does not reset stream position.
  void Stop();

  void SetLoop(bool loop);

  // Simulate stereo effect slightly delays one channel.
  void SetSimulateStereo(bool simulate);

  // Vary basic resampling for sound variations.
  void SetResampleStep(size_t value);

  // Set the current volume, max volume and volume increment steps.
  void SetAmplitude(float value);
  void SetMaxAmplitude(float value);
  void SetAmplitudeInc(float value);

  // Set a callback to be called once playback ends.
  void SetEndCallback(base::Closure cb);

  // Active AudioBus.
  std::shared_ptr<AudioBus> audio_bus;
  // AudioBus to be set as active once playback ends.
  std::shared_ptr<AudioBus> pending_audio_bus;
  // Stream position.
  size_t src_index = 0;
  size_t accumulator = 0;

  // Accessed by main thread only.
  bool streaming = false;
  base::Closure end_cb;
  base::Closure restart_cb;

  // Accessed by main thread and audio thread.
  std::atomic<unsigned> flags{0};
  std::atomic<size_t> step{100};
  std::atomic<float> amplitude{1.0f};
  std::atomic<float> amplitude_inc{0};
  std::atomic<float> max_amplitude{1.0f};

  // Accessed by audio thread and decoding worker thread.
  std::atomic<bool> streaming_in_progress{false};

 private:
  MixerInput();
};

}  // namespace eng

#endif  // ENGINE_AUDIO_MIXER_INPUT_H
