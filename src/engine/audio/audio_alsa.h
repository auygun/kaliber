#ifndef AUDIO_ALSA_H
#define AUDIO_ALSA_H

#include <atomic>
#include <thread>

#include "audio_base.h"

typedef struct _snd_pcm snd_pcm_t;

namespace eng {

class AudioResource;

class AudioAlsa : public AudioBase {
 public:
  AudioAlsa();
  ~AudioAlsa();

  bool Initialize();

  void Shutdown();

  void Suspend();
  void Resume();

  int GetHardwareSampleRate();

 private:
  // Handle for the PCM device.
  snd_pcm_t* device_;

  std::thread audio_thread_;
  std::atomic<bool> terminate_audio_thread_{false};
  std::atomic<bool> suspend_audio_thread_{false};

  size_t num_channels_ = 0;
  int sample_rate_ = 0;
  size_t period_size_ = 0;

  void StartAudioThread();
  void TerminateAudioThread();

  void AudioThreadMain();
};

}  // namespace eng

#endif  // AUDIO_ALSA_H
