#ifndef ENGINE_AUDIO_AUDIO_SINK_ALSA_H
#define ENGINE_AUDIO_AUDIO_SINK_ALSA_H

#include <atomic>
#include <thread>

#include "engine/audio/audio_sink.h"

typedef struct _snd_pcm snd_pcm_t;

namespace eng {

class AudioSinkAlsa final : public AudioSink {
 public:
  AudioSinkAlsa(AudioSink::Delegate* delegate);
  ~AudioSinkAlsa() final;

  bool Initialize() final;

  void Suspend() final;
  void Resume() final;

  size_t GetHardwareSampleRate() final;

 private:
  // Handle for the PCM device.
  snd_pcm_t* device_;

  std::thread audio_thread_;
  std::atomic<bool> terminate_audio_thread_{false};
  std::atomic<bool> suspend_audio_thread_{false};

  size_t num_channels_ = 0;
  size_t sample_rate_ = 0;
  size_t period_size_ = 0;

  AudioSink::Delegate* delegate_ = nullptr;

  void StartAudioThread();
  void TerminateAudioThread();

  void AudioThreadMain();
};

}  // namespace eng

#endif  // ENGINE_AUDIO_AUDIO_SINK_ALSA_H
