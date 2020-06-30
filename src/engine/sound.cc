#include "sound.h"

#include <array>

#include "../base/log.h"
#define MINIMP3_ONLY_MP3
#define MINIMP3_ONLY_SIMD
#define MINIMP3_FLOAT_OUTPUT
#define MINIMP3_NO_STDIO
#define MINIMP3_IMPLEMENTATION
#include "../third_party/minimp3/minimp3_ex.h"
#include "../third_party/r8b/CDSPResampler.h"
#include "engine.h"
#include "platform/asset_file.h"

using namespace base;

namespace {

constexpr size_t kMaxSamplesPerChunk = MINIMP3_MAX_SAMPLES_PER_FRAME * 10;

template <typename T>
std::array<std::unique_ptr<T[]>, 2> Deinterleave(size_t num_channels,
                                                 size_t num_samples,
                                                 float* input_buffer) {
  std::array<std::unique_ptr<T[]>, 2> channels;

  if (num_channels == 1) {
    // Single channel.
    channels[0] = std::make_unique<T[]>(num_samples);
    for (int i = 0; i < num_samples; ++i)
      channels[0].get()[i] = input_buffer[i];
  } else {
    // Deinterleave into separate channels.
    channels[0] = std::make_unique<T[]>(num_samples);
    channels[1] = std::make_unique<T[]>(num_samples);
    for (int i = 0, j = 0; i < num_samples * 2; i += 2) {
      channels[0].get()[j] = input_buffer[i];
      channels[1].get()[j++] = input_buffer[i + 1];
    }
  }

  return channels;
}

}  // namespace

namespace eng {

Sound::Sound() = default;

Sound::~Sound() {
  if (mp3_dec_)
    mp3dec_ex_close(mp3_dec_.get());
}

bool Sound::Load(const std::string& file_name, bool stream) {
  size_t buffer_size = 0;
  encoded_data_ = AssetFile::ReadWholeFile(file_name.c_str(),
                                           Engine::Get().GetRootPath().c_str(),
                                           &buffer_size, false);
  if (!encoded_data_) {
    LOG << "Failed to read file: " << file_name;
    return false;
  }

  if (mp3_dec_)
    mp3dec_ex_close(mp3_dec_.get());
  else
    mp3_dec_ = std::make_unique<mp3dec_ex_t>();

  int err = mp3dec_ex_open_buf(mp3_dec_.get(),
                               reinterpret_cast<uint8_t*>(encoded_data_.get()),
                               buffer_size, MP3D_SEEK_TO_BYTE);
  if (err) {
    LOG << "Failed to decode file: " << file_name << " error: " << err;
    return false;
  }

  is_streaming_sound_ = stream;

  LOG << (is_streaming_sound_ ? "Streaming " : "Loading ") << file_name << ". "
      << mp3_dec_->samples << " samples, " << mp3_dec_->info.channels
      << " channels, " << mp3_dec_->info.hz << " hz, "
      << "layer " << mp3_dec_->info.layer << ", "
      << "avg_bitrate_kbps " << mp3_dec_->info.bitrate_kbps;

  num_channels_ = mp3_dec_->info.channels;
  hz_ = mp3_dec_->info.hz;
  num_samples_back_ = cur_sample_back_ = 0;
  eof_ = false;

  DCHECK(num_channels_ > 0 && num_channels_ <= 2);

  size_t system_hz = Engine::Get().GetAudioSampleRate();

  if (is_streaming_sound_) {
    resampler_ = std::make_unique<r8b::CDSPResampler16>(hz_, system_hz,
                                                        kMaxSamplesPerChunk);

    // Fill up buffers.
    StreamInternal(kMaxSamplesPerChunk, false);
    SwapBuffers();
    StreamInternal(kMaxSamplesPerChunk, false);

    if (eof_) {
      // Sample is smaller than buffer. No need to stream.
      is_streaming_sound_ = false;
      mp3dec_ex_close(mp3_dec_.get());
      mp3_dec_.reset();
      encoded_data_.reset();
      resampler_.reset();
    }
  } else {
    resampler_ = std::make_unique<r8b::CDSPResampler16>(hz_, system_hz,
                                                        mp3_dec_->samples);

    // Decode entire file.
    StreamInternal(mp3_dec_->samples, false);
    SwapBuffers();
    eof_ = true;

    // We are done with decoding for non-streaming sound.
    mp3dec_ex_close(mp3_dec_.get());
    mp3_dec_.reset();
    encoded_data_.reset();
    resampler_.reset();
  }

  return true;
}

bool Sound::Stream(bool loop) {
  DCHECK(is_streaming_sound_);

  return StreamInternal(kMaxSamplesPerChunk, loop);
}

void Sound::SwapBuffers() {
  front_buffer_[0].swap(back_buffer_[0]);
  front_buffer_[1].swap(back_buffer_[1]);

  cur_sample_front_ = cur_sample_back_;
  num_samples_front_ = num_samples_back_;
  num_samples_back_ = 0;
}

void Sound::ResetStream() {
  if (is_streaming_sound_ && cur_sample_front_ != 0) {
    // Seek to 0 and ivalidate decoded data.
    mp3dec_ex_seek(mp3_dec_.get(), 0);
    eof_ = false;
    num_samples_back_ = num_samples_front_ = 0;
    cur_sample_front_ = cur_sample_back_ = 0;
  }
}

float* Sound::GetBuffer(int channel) const {
  return front_buffer_[channel].get();
}

bool Sound::StreamInternal(size_t num_samples, bool loop) {
  auto buffer = std::make_unique<float[]>(num_samples);

  cur_sample_back_ = mp3_dec_->cur_sample;

  for (;;) {
    size_t samples_read =
        mp3dec_ex_read(mp3_dec_.get(), buffer.get(), num_samples);
    if (samples_read != num_samples && mp3_dec_->last_error) {
      LOG << "mp3 decode error: " << mp3_dec_->last_error;
      eof_ = true;
      return false;
    }

    num_samples_back_ = samples_read / mp3_dec_->info.channels;
    if (!num_samples_back_ && loop) {
      mp3dec_ex_seek(mp3_dec_.get(), 0);
      loop = false;
      continue;
    }
    break;
  }

  if (num_samples_back_)
    Preprocess(std::move(buffer));
  else
    eof_ = true;

  return true;
}

void Sound::Preprocess(std::unique_ptr<float[]> input_buffer) {
  size_t system_hz = Engine::Get().GetAudioSampleRate();

  if (system_hz == hz_) {
    auto channels = Deinterleave<float>(num_channels_, num_samples_back_,
                                        input_buffer.get());

    // No need for resmapling.
    back_buffer_[0] = std::move(channels[0]);
    if (num_channels_ == 2)
      back_buffer_[1] = std::move(channels[1]);
  } else {
    // r8b resampler supports only double floating point type.
    auto channels = Deinterleave<double>(num_channels_, num_samples_back_,
                                         input_buffer.get());

    size_t resampled_num_samples =
        ((float)system_hz / (float)hz_) * num_samples_back_;

    if (!back_buffer_[0]) {
      if (max_samples_ < resampled_num_samples)
        max_samples_ = resampled_num_samples;
      back_buffer_[0] = std::make_unique<float[]>(max_samples_);
      if (num_channels_ == 2)
        back_buffer_[1] = std::make_unique<float[]>(max_samples_);
    }

    // Resample to match the system sample rate.
    for (int i = 0; i < num_channels_; ++i) {
      resampler_->oneshot(channels[i].get(), num_samples_back_,
                          back_buffer_[i].get(), resampled_num_samples);
    }
    num_samples_back_ = resampled_num_samples;
  }
}

}  // namespace eng
