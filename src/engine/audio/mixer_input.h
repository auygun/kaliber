#ifndef ENGINE_AUDIO_MIXER_INPUT_H
#define ENGINE_AUDIO_MIXER_INPUT_H

#include <atomic>
#include <memory>

#include "base/closure.h"

namespace eng {

class AudioBus;
class AudioMixer;

// An audio input stream that gets mixed and rendered to the audio device.
// Handles playback and volume control.
class MixerInput : public std::enable_shared_from_this<MixerInput> {
 public:
  enum Flags { kLoop = 1, kStopped = 2, kSimulateStereo = 4 };

  ~MixerInput();

  static std::shared_ptr<MixerInput> Create();

  bool IsValid() const { return !!audio_bus_; }

  // Set AudioBus for playback. If this MixerInput is already playing, keeps it
  // as pending and sets once playback ends.
  void SetAudioBus(std::shared_ptr<AudioBus> audio_bus);

  // Starts playback. Remembers last stream position. Resets the stream position
  // if restart is true.
  void Play(AudioMixer* mixer, bool restart);

  // Stops playback. Does not reset stream position.
  void Stop();

  // Set whether the playback should loop or not.
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

  // Getters
  std::shared_ptr<AudioBus> GetAudioBus() const { return audio_bus_; }
  unsigned GetFlags() const { return flags_.load(std::memory_order_relaxed); }
  size_t GetStep() const { return step_.load(std::memory_order_relaxed); }
  float GetAmplitude() const {
    return amplitude_.load(std::memory_order_relaxed);
  }
  float GetAmplitudeInc() const {
    return amplitude_inc_.load(std::memory_order_relaxed);
  }
  float GetMaxAmplitude() const {
    return max_amplitude_.load(std::memory_order_relaxed);
  }
  size_t GetSrcIndex() const { return src_index_; }
  size_t GetAccumulator() const { return accumulator_; }

  bool IsStreamingInProgress() const;

  // Called by the mixer to save the last sample position.
  void SetPosition(size_t index, int accumulator);

  // Called by the mixer when more data is needed.
  bool OnMoreData(bool loop);

  // Called by the mixer when playback ends.
  void OnRemovedFromMixer();

 private:
  // Active AudioBus.
  std::shared_ptr<AudioBus> audio_bus_;
  // AudioBus to be set as active once playback ends.
  std::shared_ptr<AudioBus> pending_audio_bus_;
  // Stream position.
  size_t src_index_ = 0;
  size_t accumulator_ = 0;

  // Accessed by main thread only.
  bool playing_ = false;
  base::Closure end_cb_;
  base::Closure restart_cb_;

  // Accessed by main thread and audio thread.
  std::atomic<unsigned> flags_{0};
  std::atomic<size_t> step_{100};
  std::atomic<float> amplitude_{1.0f};
  std::atomic<float> amplitude_inc_{0};
  std::atomic<float> max_amplitude_{1.0f};

  // Accessed by audio thread and decoding worker thread.
  std::atomic<bool> streaming_in_progress_{false};

  MixerInput();
};

}  // namespace eng

#endif  // ENGINE_AUDIO_MIXER_INPUT_H
