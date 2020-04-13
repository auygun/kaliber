#include "audio_alsa.h"

#include <alsa/asoundlib.h>

#include "../../base/log.h"
#include "audio_resource.h"

using namespace base;

namespace eng {

AudioAlsa::AudioAlsa() = default;

AudioAlsa::~AudioAlsa() = default;

bool AudioAlsa::Initialize() {
  LOG << "Initializing audio system.";

  int err;

  // Contains information about the hardware.
  snd_pcm_hw_params_t* hw_params;

  // "default" is usualy PulseAudio. Use "plughw:CARD=PCH" instead for direct
  // hardware device with software format conversion.
  if ((err = snd_pcm_open(&pcm_handle_, "plughw:CARD=PCH",
                          SND_PCM_STREAM_PLAYBACK, 0)) < 0) {
    LOG << "Cannot open audio device. Error: " << snd_strerror(err);
    return false;
  }

  do {
    // Allocate the snd_pcm_hw_params_t structure on the stack.
    snd_pcm_hw_params_alloca(&hw_params);

    // Init hw_params with full configuration space.
    if ((err = snd_pcm_hw_params_any(pcm_handle_, hw_params)) < 0) {
      LOG << "Cannot initialize hardware parameter structure. Error: "
          << snd_strerror(err);
      break;
    }

    if ((err = snd_pcm_hw_params_set_access(
             pcm_handle_, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
      LOG << "Cannot set access type. Error: " << snd_strerror(err);
      break;
    }

    if ((err = snd_pcm_hw_params_set_format(pcm_handle_, hw_params,
                                            SND_PCM_FORMAT_FLOAT)) < 0) {
      LOG << "Cannot set sample format. Error: " << snd_strerror(err);
      break;
    }

    // Disable software resampler.
    if ((err = snd_pcm_hw_params_set_rate_resample(pcm_handle_, hw_params, 0)) <
        0) {
      LOG << "Cannot disbale software resampler. Error: " << snd_strerror(err);
      break;
    }

    unsigned sample_rate = 48000;
    if ((err = snd_pcm_hw_params_set_rate_near(pcm_handle_, hw_params,
                                               &sample_rate, 0)) < 0) {
      LOG << "Cannot set sample rate. Error: " << snd_strerror(err);
      break;
    }

    if ((err = snd_pcm_hw_params_set_channels(pcm_handle_, hw_params, 2)) < 0) {
      LOG << "Cannot set channel count. Error: " << snd_strerror(err);
      break;
    }

    // Set period time to 4 ms. The latency will be 12 ms for 3 perods.
    unsigned period_time = 4000;
    if ((err = snd_pcm_hw_params_set_period_time_near(pcm_handle_, hw_params,
                                                      &period_time, 0)) < 0) {
      LOG << "Cannot set periods. Error: " << snd_strerror(err);
      break;
    }

    unsigned periods = 3;
    if ((err = snd_pcm_hw_params_set_periods_near(pcm_handle_, hw_params,
                                                  &periods, 0)) < 0) {
      LOG << "Cannot set periods. Error: " << snd_strerror(err);
      break;
    }

    // Apply HW parameter settings to PCM device and prepare device.
    if ((err = snd_pcm_hw_params(pcm_handle_, hw_params)) < 0) {
      LOG << "Cannot set parameters. Error: " << snd_strerror(err);
      break;
    }

    if ((err = snd_pcm_prepare(pcm_handle_)) < 0) {
      LOG << "Cannot prepare audio interface for use. Error: "
          << snd_strerror(err);
      break;
    }

    snd_pcm_access_t access;
    unsigned num_channels;
    snd_pcm_format_t format;
    snd_pcm_uframes_t period_size;
    snd_pcm_uframes_t buffer_size;

    snd_pcm_hw_params_get_access(hw_params, &access);
    snd_pcm_hw_params_get_channels(hw_params, &num_channels);
    snd_pcm_hw_params_get_format(hw_params, &format);
    snd_pcm_hw_params_get_period_size(hw_params, &period_size, nullptr);
    snd_pcm_hw_params_get_period_time(hw_params, &period_time, nullptr);
    snd_pcm_hw_params_get_periods(hw_params, &periods, nullptr);
    snd_pcm_hw_params_get_buffer_size(hw_params, &buffer_size);

    LOG << "Alsa Audio:";
    LOG << "  access:        " << snd_pcm_access_name(access);
    LOG << "  format:        " << snd_pcm_format_name(format);
    LOG << "  channel count: " << num_channels;
    LOG << "  sample rate:   " << sample_rate;
    LOG << "  period size:   " << period_size;
    LOG << "  period time:   " << period_time;
    LOG << "  periods:       " << periods;
    LOG << "  buffer_size:   " << buffer_size;

    num_channels_ = num_channels;
    sample_rate_ = sample_rate;
    period_size_ = period_size;

    StartWorker();

    return true;
  } while (false);

  snd_pcm_close(pcm_handle_);
  return false;
}

void AudioAlsa::Shutdown() {
  LOG << "Shutting down audio system.";
  TerminateWorker();
  snd_pcm_drop(pcm_handle_);
  snd_pcm_close(pcm_handle_);
}

size_t AudioAlsa::GetSampleRate() {
  return sample_rate_;
}

bool AudioAlsa::StartWorker() {
  LOG << "Starting audio thread.";

  std::promise<bool> promise;
  std::future<bool> future = promise.get_future();
  worker_thread_ =
      std::thread(&AudioAlsa::WorkerMain, this, std::move(promise));
  return future.get();
}

void AudioAlsa::TerminateWorker() {
  // Notify worker thread and wait for it to terminate.
  if (terminate_worker_)
    return;
  terminate_worker_ = true;
  LOG << "Terminating audio thread";
  worker_thread_.join();
}

void AudioAlsa::WorkerMain(std::promise<bool> promise) {
  promise.set_value(true);

  size_t num_frames = period_size_ / (num_channels_ * sizeof(float));
  auto buffer = std::make_unique<float[]>(num_frames * 2);

  for (;;) {
    if (terminate_worker_)
      return;

    RenderAudio(buffer.get(), num_frames);

    while (snd_pcm_writei(pcm_handle_, buffer.get(), num_frames) < 0) {
      snd_pcm_prepare(pcm_handle_);
      LOG << "Audio buffer underrun!";
    }
  }
}

}  // namespace eng
