#include "engine/audio/audio_bus.h"

#include "base/log.h"
#include "engine/audio/sinc_resampler.h"
#include "engine/engine.h"

using namespace base;

namespace eng {

namespace {

template <typename T>
std::array<std::unique_ptr<T[]>, 2> Deinterleave(
    size_t num_channels,
    size_t num_samples,
    std::unique_ptr<float[]> source_buffer) {
  std::array<std::unique_ptr<T[]>, 2> channels;

  if (num_channels == 1) {
    if constexpr (std::is_same<float, T>::value) {
      // Passthrough
      channels[0] = std::move(source_buffer);
    } else {
      channels[0] = std::make_unique<T[]>(num_samples);
      for (int i = 0; i < num_samples; ++i)
        channels[0].get()[i] = static_cast<T>(source_buffer.get()[i]);
    }
  } else {
    // Deinterleave into separate channels.
    channels[0] = std::make_unique<T[]>(num_samples);
    channels[1] = std::make_unique<T[]>(num_samples);
    for (size_t i = 0, j = 0; i < num_samples * 2; i += 2) {
      channels[0].get()[j] = static_cast<T>(source_buffer.get()[i]);
      channels[1].get()[j++] = static_cast<T>(source_buffer.get()[i + 1]);
    }
  }

  return channels;
}

std::unique_ptr<SincResampler> CreateResampler(int src_samle_rate,
                                               int dst_sample_rate,
                                               size_t num_samples) {
  const double io_ratio = static_cast<double>(src_samle_rate) /
                          static_cast<double>(dst_sample_rate);
  auto resampler = std::make_unique<SincResampler>(io_ratio, num_samples);
  resampler->PrimeWithSilence();
  return resampler;
}

}  // namespace

AudioBus::AudioBus() = default;
AudioBus::~AudioBus() = default;

void AudioBus::SetAudioConfig(size_t num_channels, size_t sample_rate) {
  num_channels_ = num_channels;
  sample_rate_ = sample_rate;
}

void AudioBus::FromInterleaved(std::unique_ptr<float[]> source_buffer,
                               size_t samples_per_channel) {
  auto channels = Deinterleave<float>(num_channels_, samples_per_channel,
                                      std::move(source_buffer));

  size_t hw_sample_rate = Engine::Get().GetAudioHardwareSampleRate();

  if (hw_sample_rate == sample_rate_) {
    // Passthrough
    channel_data_[0] = std::move(channels[0]);
    if (num_channels_ == 2)
      channel_data_[1] = std::move(channels[1]);
    samples_per_channel_ = samples_per_channel;
  } else {
    if (!resampler_[0]) {
      for (size_t i = 0; i < num_channels_; ++i) {
        resampler_[i] =
            CreateResampler(sample_rate_, hw_sample_rate, samples_per_channel);
      }
    }

    size_t num_resampled_samples =
        (size_t)(((float)hw_sample_rate / (float)sample_rate_) *
                 samples_per_channel);
    DCHECK(num_resampled_samples <= (size_t)resampler_[0]->ChunkSize());

    if (!channel_data_[0]) {
      channel_data_[0] = std::make_unique<float[]>(num_resampled_samples);
      if (num_channels_ == 2)
        channel_data_[1] = std::make_unique<float[]>(num_resampled_samples);
    }
    samples_per_channel_ = num_resampled_samples;

    // Resample to match the system sample rate.
    for (size_t i = 0; i < num_channels_; ++i) {
      resampler_[i]->Resample(num_resampled_samples, channel_data_[i].get(),
                              [&](int frames, float* destination) {
                                memcpy(destination, channels[i].get(),
                                       frames * sizeof(float));
                              });
    }

    if (EndOfStream()) {
      // We are done with the resampler.
      for (size_t i = 0; i < num_channels_; ++i)
        resampler_[i].reset();
    }
  }
}

}  // namespace eng