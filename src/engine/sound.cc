#include "sound.h"

#include <cassert>

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

constexpr size_t kMinSamplesForStreaming = 500000;
constexpr size_t kMaxSamplesPerDecode = MINIMP3_MAX_SAMPLES_PER_FRAME * 50;

}  // namespace

namespace eng {

Sound::Sound() = default;

Sound::~Sound() {
  if (mp3_dec_)
    mp3dec_ex_close(mp3_dec_.get());
}

bool Sound::Load(const std::string& file_name) {
  assert(!IsImmutable());

  SetName(file_name);

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
  mp3_dec_ = std::make_unique<mp3dec_ex_t>();

  int err = mp3dec_ex_open_buf(mp3_dec_.get(),
                               reinterpret_cast<uint8_t*>(encoded_data_.get()),
                               buffer_size, MP3D_SEEK_TO_BYTE);
  if (err) {
    LOG << "Failed to decode file: " << file_name << " error: " << err;
    return false;
  }

  is_streaming_sound_ =
      mp3_dec_->samples / mp3_dec_->info.channels > kMinSamplesForStreaming
          ? true
          : false;

  LOG << (is_streaming_sound_ ? "Streaming " : "Loading ") << GetName() << ". "
      << mp3_dec_->samples << " samples, " << mp3_dec_->info.channels
      << " channels, " << mp3_dec_->info.hz << " hz, "
      << "layer " << mp3_dec_->info.layer << ", "
      << "avg_bitrate_kbps " << mp3_dec_->info.bitrate_kbps;

  num_channels_ = mp3_dec_->info.channels;
  hz_ = mp3_dec_->info.hz;
  num_samples_back_ = 0;
  eof_ = false;

  assert(num_channels_ > 0 && num_channels_ <= 2);

  // Fill up buffers.
  if (is_streaming_sound_) {
    StreamInternal(kMaxSamplesPerDecode, false);
    SwapBuffersInternal();
    StreamInternal(kMaxSamplesPerDecode, false);
  } else {
    StreamInternal(mp3_dec_->samples, false);
    SwapBuffersInternal();
    eof_ = true;
    encoded_data_.reset();
  }

  return true;
}

bool Sound::Stream(bool loop) {
  assert(!IsImmutable());
  assert(is_streaming_sound_);

  return StreamInternal(kMaxSamplesPerDecode, loop);
}

void Sound::SwapBuffers() {
  assert(!IsImmutable());
  assert(is_streaming_sound_);

  SwapBuffersInternal();

  // Memory barrier to ensure all memory writes become visible to the decoder
  // thread.
  streaming_in_progress_.store(true, std::memory_order_release);
}

size_t Sound::IsStreamingInProgress() const {
  assert(is_streaming_sound_);

  return streaming_in_progress_.load(std::memory_order_acquire);
}

size_t Sound::GetSize() const {
  return num_samples_front_ * sizeof(mp3d_sample_t);
}

float* Sound::GetBuffer(int channel) {
  assert(!IsImmutable());

  return front_buffer_[channel].get();
}

bool Sound::StreamInternal(size_t num_samples, bool loop) {
  auto buffer = std::make_unique<float[]>(num_samples);

  for (;;) {
    size_t samples_read =
        mp3dec_ex_read(mp3_dec_.get(), buffer.get(), num_samples);
    if (samples_read != num_samples && mp3_dec_->last_error) {
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

  // Memory barrier to ensure all memory writes become visible to the audio
  // thread.
  streaming_in_progress_.store(false, std::memory_order_release);

  return true;
}

void Sound::SwapBuffersInternal() {
  front_buffer_[0].swap(back_buffer_[0]);
  front_buffer_[1].swap(back_buffer_[1]);

  num_samples_front_ = num_samples_back_;
  num_samples_back_ = 0;
}

void Sound::Preprocess(std::unique_ptr<float[]> input_buffer) {
  if (num_channels_ == 1) {
    back_buffer_[0] = std::move(input_buffer);
  } else {
    // Deinterleave into separate channels.
    back_buffer_[0] = std::make_unique<float[]>(num_samples_back_);
    back_buffer_[1] = std::make_unique<float[]>(num_samples_back_);
    for (int i = 0, j = 0; i < num_samples_back_ * 2; i += 2) {
      back_buffer_[0].get()[j] = input_buffer.get()[i];
      back_buffer_[1].get()[j++] = input_buffer.get()[i + 1];
    }
  }

  // Resample to match the system sample rate if needed.
  size_t system_hz = Engine::Get().GetAudioSampleRate();
  if (system_hz == 0 || system_hz == hz_)
    return;

  size_t resampled_num_samples =
      ((float)system_hz / (float)hz_) * num_samples_back_;

  r8b::CDSPResampler24 resampler(hz_, system_hz, num_samples_back_);

  for (int i = 0; i < num_channels_; ++i) {
    auto resampled_buffer_ = std::make_unique<float[]>(resampled_num_samples);
    resampler.oneshot(back_buffer_[i].get(), num_samples_back_,
                      resampled_buffer_.get(), resampled_num_samples);
    back_buffer_[i].swap(resampled_buffer_);
  }
  num_samples_back_ = resampled_num_samples;
}

}  // namespace eng
