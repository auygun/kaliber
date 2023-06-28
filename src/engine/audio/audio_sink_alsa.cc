#include "engine/audio/audio_sink_alsa.h"

#include <memory>

#include <alsa/asoundlib.h>

#include "base/log.h"

using namespace base;

namespace eng {

AudioSinkAlsa::AudioSinkAlsa(AudioSink::Delegate* delegate)
    : delegate_(delegate) {}

AudioSinkAlsa::~AudioSinkAlsa() {
  LOG(0) << "Shutting down audio.";

  TerminateAudioThread();
  snd_pcm_drop(device_);
  snd_pcm_close(device_);
}

bool AudioSinkAlsa::Initialize() {
  LOG(0) << "Initializing audio.";

  int err;

  // Contains information about the hardware.
  snd_pcm_hw_params_t* hw_params;

  // TODO: "default" is usualy PulseAudio. Select a device with "plughw" for
  // direct hardware device with software format conversion.
  if ((err = snd_pcm_open(&device_, "default", SND_PCM_STREAM_PLAYBACK, 0)) <
      0) {
    LOG(0) << "Cannot open audio device. Error: " << snd_strerror(err);
    return false;
  }

  do {
    // Allocate the snd_pcm_hw_params_t structure on the stack.
    snd_pcm_hw_params_alloca(&hw_params);

    // Init hw_params with full configuration space.
    if ((err = snd_pcm_hw_params_any(device_, hw_params)) < 0) {
      LOG(0) << "Cannot initialize hardware parameter structure. Error: "
             << snd_strerror(err);
      break;
    }

    if ((err = snd_pcm_hw_params_set_access(
             device_, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
      LOG(0) << "Cannot set access type. Error: " << snd_strerror(err);
      break;
    }

    if ((err = snd_pcm_hw_params_set_format(device_, hw_params,
                                            SND_PCM_FORMAT_FLOAT)) < 0) {
      LOG(0) << "Cannot set sample format. Error: " << snd_strerror(err);
      break;
    }

    // Disable software resampler.
    if ((err = snd_pcm_hw_params_set_rate_resample(device_, hw_params, 0)) <
        0) {
      LOG(0) << "Cannot disbale software resampler. Error: "
             << snd_strerror(err);
      break;
    }

    unsigned sample_rate = 48000;
    if ((err = snd_pcm_hw_params_set_rate_near(device_, hw_params, &sample_rate,
                                               0)) < 0) {
      LOG(0) << "Cannot set sample rate. Error: " << snd_strerror(err);
      break;
    }

    if ((err = snd_pcm_hw_params_set_channels(device_, hw_params, 2)) < 0) {
      LOG(0) << "Cannot set channel count. Error: " << snd_strerror(err);
      break;
    }

    // Set period time to 4 ms. The latency will be 12 ms for 3 perods.
    unsigned period_time = 4000;
    if ((err = snd_pcm_hw_params_set_period_time_near(device_, hw_params,
                                                      &period_time, 0)) < 0) {
      LOG(0) << "Cannot set periods. Error: " << snd_strerror(err);
      break;
    }

    unsigned periods = 3;
    if ((err = snd_pcm_hw_params_set_periods_near(device_, hw_params, &periods,
                                                  0)) < 0) {
      LOG(0) << "Cannot set periods. Error: " << snd_strerror(err);
      break;
    }

    // Apply HW parameter settings to PCM device and prepare device.
    if ((err = snd_pcm_hw_params(device_, hw_params)) < 0) {
      LOG(0) << "Cannot set parameters. Error: " << snd_strerror(err);
      break;
    }

    if ((err = snd_pcm_prepare(device_)) < 0) {
      LOG(0) << "Cannot prepare audio interface for use. Error: "
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

    LOG(0) << "Alsa Audio:";
    LOG(0) << "  access:        " << snd_pcm_access_name(access);
    LOG(0) << "  format:        " << snd_pcm_format_name(format);
    LOG(0) << "  channel count: " << num_channels;
    LOG(0) << "  sample rate:   " << sample_rate;
    LOG(0) << "  period size:   " << period_size;
    LOG(0) << "  period time:   " << period_time;
    LOG(0) << "  periods:       " << periods;
    LOG(0) << "  buffer_size:   " << buffer_size;

    num_channels_ = num_channels;
    sample_rate_ = sample_rate;
    period_size_ = period_size;

    StartAudioThread();

    return true;
  } while (false);

  snd_pcm_close(device_);
  return false;
}

void AudioSinkAlsa::Suspend() {
  suspend_audio_thread_.store(true, std::memory_order_relaxed);
}

void AudioSinkAlsa::Resume() {
  suspend_audio_thread_.store(false, std::memory_order_relaxed);
}

size_t AudioSinkAlsa::GetHardwareSampleRate() {
  return sample_rate_;
}

void AudioSinkAlsa::StartAudioThread() {
  DCHECK(!audio_thread_.joinable());

  LOG(0) << "Starting audio thread.";
  terminate_audio_thread_.store(false, std::memory_order_relaxed);
  suspend_audio_thread_.store(false, std::memory_order_relaxed);
  audio_thread_ = std::thread(&AudioSinkAlsa::AudioThreadMain, this);
}

void AudioSinkAlsa::TerminateAudioThread() {
  if (!audio_thread_.joinable())
    return;

  LOG(0) << "Terminating audio thread";
  terminate_audio_thread_.store(true, std::memory_order_relaxed);
  suspend_audio_thread_.store(true, std::memory_order_relaxed);
  audio_thread_.join();
}

void AudioSinkAlsa::AudioThreadMain() {
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
      DLOG(0) << "Alsa buffer underrun!";
    }
  }
}

}  // namespace eng
