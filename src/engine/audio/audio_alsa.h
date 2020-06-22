#ifndef AUDIO_ALSA_H
#define AUDIO_ALSA_H

#include <future>
#include <memory>
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

  size_t GetSampleRate();

 private:
  // Handle for the PCM device.
  snd_pcm_t* pcm_handle_;

  std::thread worker_thread_;
  bool terminate_worker_ = false;

  size_t num_channels_ = 0;
  size_t sample_rate_ = 0;
  size_t period_size_ = 0;

  bool StartWorker();
  void TerminateWorker();

  void WorkerMain(std::promise<bool> promise);
};

}  // namespace eng

#endif  // AUDIO_ALSA_H
