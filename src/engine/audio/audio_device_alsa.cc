#include "engine/audio/audio_device_alsa.h"

#include <memory>

#include <alsa/asoundlib.h>

#include "base/log.h"
#include "base/mem.h"
#include "base/timer.h"

using namespace base;

namespace eng {

AudioDeviceAlsa::AudioDeviceAlsa(AudioDevice::Delegate* delegate)
    : delegate_(delegate) {}

AudioDeviceAlsa::~AudioDeviceAlsa() {
  LOG(0) << "Shutting down audio.";

  TerminateAudioThread();
  snd_pcm_drop(device_);
  snd_pcm_close(device_);
}

bool AudioDeviceAlsa::Initialize() {
  DeviceNames device_names = GetAlsaAudioDevices(kStreamPlayback);
  for (auto& dn : device_names)
    DLOG(0) << dn.device_name << " : " << dn.unique_id;

  LOG(0) << "Initializing audio.";

  int err;

  // Contains information about the hardware.
  snd_pcm_hw_params_t* hw_params;

  // TODO: "default" is usualy PulseAudio. Select a device with "plughw" for
  // direct hardware device with software format conversion.

  // "default" is a virtual device mapped to real device exclusively opened by
  // PulseAudio, so we must open the device via the "default" moniker. If
  // PulseAudio is not available, a device id must be picked from the device
  // list above.
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

    // Set period time to 4 ms. The latency will be 12 ms for 3 periods.
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

void AudioDeviceAlsa::Suspend() {
  suspend_audio_thread_.store(true, std::memory_order_relaxed);
}

void AudioDeviceAlsa::Resume() {
  suspend_audio_thread_.store(false, std::memory_order_relaxed);
}

size_t AudioDeviceAlsa::GetHardwareSampleRate() {
  return sample_rate_;
}

void AudioDeviceAlsa::StartAudioThread() {
  DCHECK(!audio_thread_.joinable());

  LOG(0) << "Starting audio thread.";
  terminate_audio_thread_.store(false, std::memory_order_relaxed);
  suspend_audio_thread_.store(false, std::memory_order_relaxed);
  audio_thread_ = std::thread(&AudioDeviceAlsa::AudioThreadMain, this);
}

void AudioDeviceAlsa::TerminateAudioThread() {
  if (!audio_thread_.joinable())
    return;

  LOG(0) << "Terminating audio thread";
  terminate_audio_thread_.store(true, std::memory_order_relaxed);
  suspend_audio_thread_.store(true, std::memory_order_relaxed);
  audio_thread_.join();
}

void AudioDeviceAlsa::AudioThreadMain() {
  DCHECK(delegate_);

  int err;
  size_t num_frames = period_size_ / (num_channels_ * sizeof(float));
  auto buffer = std::make_unique<float[]>(num_frames * 2);

  for (;;) {
    if (suspend_audio_thread_.load(std::memory_order_relaxed)) {
      snd_pcm_drain(device_);
      while (suspend_audio_thread_.load(std::memory_order_relaxed)) {
        if (terminate_audio_thread_.load(std::memory_order_relaxed))
          return;
        Sleep(1);  // Avoid busy-looping.
      }
      snd_pcm_prepare(device_);
    }

    delegate_->RenderAudio(buffer.get(), num_frames);

    while ((err = snd_pcm_writei(device_, buffer.get(), num_frames)) < 0) {
      DLOG(0) << "snd_pcm_writei: " << snd_strerror(err);
      snd_pcm_prepare(device_);
    }
  }
}

AudioDeviceAlsa::DeviceNames AudioDeviceAlsa::GetAlsaAudioDevices(
    StreamType type) {
  // Constants specified by the ALSA API for device hints.
  static const char kPcmInterfaceName[] = "pcm";
  int card = -1;
  DeviceNames device_names;

  // Loop through the sound cards to get ALSA device hints.
  while (!snd_card_next(&card) && card >= 0) {
    void** hints = NULL;
    int error = snd_device_name_hint(card, kPcmInterfaceName, &hints);
    if (!error) {
      GetAlsaDevicesInfo(type, hints, &device_names);

      // Destroy the hints now that we're done with it.
      snd_device_name_free_hint(hints);
    } else {
      DLOG(0) << "GetAlsaAudioDevices: unable to get device hints: "
              << snd_strerror(error);
    }
  }

  return device_names;
}

void AudioDeviceAlsa::GetAlsaDevicesInfo(AudioDeviceAlsa::StreamType type,
                                         void** hints,
                                         DeviceNames* device_names) {
  static const char kIoHintName[] = "IOID";
  static const char kNameHintName[] = "NAME";
  static const char kDescriptionHintName[] = "DESC";

  const char* unwanted_device_type =
      (type == kStreamPlayback ? "Input" : "Output");

  for (void** hint_iter = hints; *hint_iter != NULL; hint_iter++) {
    // Only examine devices of the right type.  Valid values are
    // "Input", "Output", and NULL which means both input and output.
    MallocAllocatedMemPtr<char> io(
        snd_device_name_get_hint(*hint_iter, kIoHintName));
    if (io != NULL && strcmp(unwanted_device_type, io.get()) == 0)
      continue;

    // Get the unique device name for the device.
    MallocAllocatedMemPtr<char> unique_device_name(
        snd_device_name_get_hint(*hint_iter, kNameHintName));

    if (unique_device_name) {
      // Get the description for the device.
      MallocAllocatedMemPtr<char> desc(
          snd_device_name_get_hint(*hint_iter, kDescriptionHintName));

      DeviceName name;
      name.unique_id = unique_device_name.get();
      if (desc) {
        // Use the more user friendly description as name.
        // Replace '\n' with '-'.
        char* pret = strchr(desc.get(), '\n');
        if (pret)
          *pret = '-';
        name.device_name = desc.get();
      } else {
        // Virtual devices don't necessarily have descriptions.
        // Use their names instead.
        name.device_name = unique_device_name.get();
      }

      // Store the device information.
      device_names->push_back(name);
    }
  }
}

}  // namespace eng
