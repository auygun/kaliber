#include "engine/audio/audio_driver_alsa.h"

#include <memory>

#include <alsa/asoundlib.h>

#include "base/log.h"
#include "engine/audio/audio_driver_delegate.h"

using namespace base;

namespace eng {

AudioDriverAlsa::AudioDriverAlsa(AudioDriverDelegate* delegate)
    : delegate_(delegate) {}

AudioDriverAlsa::~AudioDriverAlsa() {
  LOG << "Shutting down audio.";

  TerminateAudioThread();
  snd_pcm_drop(device_);
  snd_pcm_close(device_);
}

bool AudioDriverAlsa::Initialize() {
  LOG << "Initializing audio.";

  int err;

  // Contains information about the hardware.
  snd_pcm_hw_params_t* hw_params;

  // TODO: "default" is usualy PulseAudio. Select a device with "plughw" for
  // direct hardware device with software format conversion.
  if ((err = snd_pcm_open(&device_, "default", SND_PCM_STREAM_PLAYBACK, 0)) <
      0) {
    LOG << "Cannot open audio device. Error: " << snd_strerror(err);
    return false;
  }

  do {
    // Allocate the snd_pcm_hw_params_t structure on the stack.
    snd_pcm_hw_params_alloca(&hw_params);

    // Init hw_params with full configuration space.
    if ((err = snd_pcm_hw_params_any(device_, hw_params)) < 0) {
      LOG << "Cannot initialize hardware parameter structure. Error: "
          << snd_strerror(err);
      break;
    }

    if ((err = snd_pcm_hw_params_set_access(
             device_, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
      LOG << "Cannot set access type. Error: " << snd_strerror(err);
      break;
    }

    if ((err = snd_pcm_hw_params_set_format(device_, hw_params,
                                            SND_PCM_FORMAT_FLOAT)) < 0) {
      LOG << "Cannot set sample format. Error: " << snd_strerror(err);
      break;
    }

    // Disable software resampler.
    if ((err = snd_pcm_hw_params_set_rate_resample(device_, hw_params, 0)) <
        0) {
      LOG << "Cannot disbale software resampler. Error: " << snd_strerror(err);
      break;
    }

    unsigned sample_rate = 48000;
    if ((err = snd_pcm_hw_params_set_rate_near(device_, hw_params, &sample_rate,
                                               0)) < 0) {
      LOG << "Cannot set sample rate. Error: " << snd_strerror(err);
      break;
    }

    if ((err = snd_pcm_hw_params_set_channels(device_, hw_params, 2)) < 0) {
      LOG << "Cannot set channel count. Error: " << snd_strerror(err);
      break;
    }

    // Set period time to 4 ms. The latency will be 12 ms for 3 perods.
    unsigned period_time = 4000;
    if ((err = snd_pcm_hw_params_set_period_time_near(device_, hw_params,
                                                      &period_time, 0)) < 0) {
      LOG << "Cannot set periods. Error: " << snd_strerror(err);
      break;
    }

    unsigned periods = 3;
    if ((err = snd_pcm_hw_params_set_periods_near(device_, hw_params, &periods,
                                                  0)) < 0) {
      LOG << "Cannot set periods. Error: " << snd_strerror(err);
      break;
    }

    // Apply HW parameter settings to PCM device and prepare device.
    if ((err = snd_pcm_hw_params(device_, hw_params)) < 0) {
      LOG << "Cannot set parameters. Error: " << snd_strerror(err);
      break;
    }

    if ((err = snd_pcm_prepare(device_)) < 0) {
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

    StartAudioThread();

    return true;
  } while (false);

  snd_pcm_close(device_);
  return false;
}

void AudioDriverAlsa::Suspend() {
  suspend_audio_thread_.store(true, std::memory_order_relaxed);
}

void AudioDriverAlsa::Resume() {
  suspend_audio_thread_.store(false, std::memory_order_relaxed);
}

int AudioDriverAlsa::GetHardwareSampleRate() {
  return sample_rate_;
}

void AudioDriverAlsa::StartAudioThread() {
  DCHECK(!audio_thread_.joinable());

  LOG << "Starting audio thread.";
  terminate_audio_thread_.store(false, std::memory_order_relaxed);
  suspend_audio_thread_.store(false, std::memory_order_relaxed);
  audio_thread_ = std::thread(&AudioDriverAlsa::AudioThreadMain, this);
}

void AudioDriverAlsa::TerminateAudioThread() {
  if (!audio_thread_.joinable())
    return;

  LOG << "Terminating audio thread";
  terminate_audio_thread_.store(true, std::memory_order_relaxed);
  suspend_audio_thread_.store(true, std::memory_order_relaxed);
  audio_thread_.join();
}

void AudioDriverAlsa::AudioThreadMain() {
  DCHECK(delegate_);

  size_t num_frames = period_size_ / (num_channels_ * sizeof(float));
  auto buffer = std::make_unique<float[]>(num_frames * 2);

  for (;;) {
    while (suspend_audio_thread_.load(std::memory_order_relaxed)) {
      if (terminate_audio_thread_.load(std::memory_order_relaxed))
        return;
      // Avoid busy-looping.
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    delegate_->RenderAudio(buffer.get(), num_frames);

    while (snd_pcm_writei(device_, buffer.get(), num_frames) < 0) {
      snd_pcm_prepare(device_);
      DLOG << "Alsa buffer underrun!";
    }
  }
}

}  // namespace eng
