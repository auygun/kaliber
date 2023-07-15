#ifndef ENGINE_AUDIO_AUDIO_MIXER_H
#define ENGINE_AUDIO_AUDIO_MIXER_H

#include <list>
#include <memory>
#include <mutex>

#include "base/closure.h"
#include "engine/audio/audio_sink.h"

namespace base {
class TaskRunner;
}

namespace eng {

class MixerInput;

// Mix and render audio with low overhead. The mixer has zero or more inputs
// which can be added at any time. The mixer will pull from each input source
// when it needs more data. Input source will be removed once end-of-stream is
// reached. Any unfilled frames will be filled with silence. The mixer always
// outputs audio when active, even if input sources underflow. A platform
// specific AudioSink implementation is expected to periodically call
// RenderAudio() in a background thread.
class AudioMixer : public AudioSink::Delegate {
 public:
  AudioMixer();
  ~AudioMixer();

  void AddInput(std::shared_ptr<MixerInput> mixer_input);

  void SetEnableAudio(bool enable) { audio_enabled_ = enable; }
  bool IsAudioEnabled() const { return audio_enabled_; }

  void Suspend();
  void Resume();

  size_t GetHardwareSampleRate();

 private:
  static constexpr int kChannelCount = 2;

  std::list<std::shared_ptr<MixerInput>> inputs_[2];
  std::mutex lock_;

  std::list<std::shared_ptr<MixerInput>> removed_inputs_;

  std::shared_ptr<base::TaskRunner> main_thread_task_runner_;

  std::unique_ptr<AudioSink> audio_sink_;

  bool audio_enabled_ = true;

  // AudioSink::Delegate interface
  int GetChannelCount() final { return kChannelCount; }
  void RenderAudio(float* output_buffer, size_t num_frames) final;

  AudioMixer(const AudioMixer&) = delete;
  AudioMixer& operator=(const AudioMixer&) = delete;
};

}  // namespace eng

#endif  // ENGINE_AUDIO_AUDIO_MIXER_H
