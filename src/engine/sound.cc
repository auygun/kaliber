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
  auto file_buffer = AssetFile::ReadWholeFile(
      file_name.c_str(), Engine::Get().GetRootPath().c_str(), &buffer_size,
      false);
  if (!file_buffer) {
    LOG << "Failed to read file: " << file_name;
    return false;
  }

  if (mp3_dec_)
    mp3dec_ex_close(mp3_dec_.get());
  mp3_dec_ = std::make_unique<mp3dec_ex_t>();

  int err =
      mp3dec_ex_open_buf(mp3_dec_.get(),
                         reinterpret_cast<uint8_t*>(file_buffer.get()),
                         buffer_size, MP3D_SEEK_TO_BYTE);
  if (err) {
    LOG << "Failed to decode file: " << file_name << " error: " << err;
    return false;
  }

  LOG << "Decoded " << GetName() << ". " << mp3_dec_->samples << " samples, "
      << mp3_dec_->detected_samples << " samples detected, "
      << mp3_dec_->info.channels << " channels, " << mp3_dec_->info.hz << " hz, "
      << "layer " << mp3_dec_->info.layer << ", "
      << "avg_bitrate_kbps " << mp3_dec_->info.bitrate_kbps;

  num_channels_ = mp3_dec_->info.channels;
  hz_ = mp3_dec_->info.hz;
  num_samples_ = 0;

  assert(num_channels_ > 0 && num_channels_ <= 2);

  // Fill up buffer and front buffer.
  DecodeNextFrame();
  DecodeNextFrame();

  return true;
}

bool Sound::DecodeNextFrame() {
  front_buffer_[0].swap(buffer_[0]);
  front_buffer_[1].swap(buffer_[1]);

  if (num_samples_front_ && !num_samples_) {
    num_samples_front_ = 0;
    return true;
  }

  num_samples_front_ = num_samples_;
  num_samples_ = 0;

  auto buffer = std::make_unique<float[]>(MINIMP3_MAX_SAMPLES_PER_FRAME);
  size_t samples_read = mp3dec_ex_read(mp3_dec_.get(), buffer.get(), MINIMP3_MAX_SAMPLES_PER_FRAME);
  LOG << __func__ << " samples_read: " << samples_read;
  if (samples_read != MINIMP3_MAX_SAMPLES_PER_FRAME && mp3_dec_->last_error)
    return false;

  num_samples_ = samples_read / mp3_dec_->info.channels;

  if (num_samples_)
    Preprocess(std::move(buffer));

  return true;
}

size_t Sound::GetSize() const {
  return num_samples_ * sizeof(mp3d_sample_t);
}

float* Sound::GetBuffer(int channel) {
  assert(!IsImmutable());

  return front_buffer_[channel].get();
}

void Sound::Preprocess(std::unique_ptr<float[]> input_buffer) {
  if (num_channels_ == 1) {
    buffer_[0] = std::move(input_buffer);
  } else {
    // Deinterleave into separate channels.
    buffer_[0] = std::make_unique<float[]>(num_samples_);
    buffer_[1] = std::make_unique<float[]>(num_samples_);
    for (int i = 0, j = 0; i < num_samples_ * 2; i += 2) {
      buffer_[0].get()[j] = input_buffer.get()[i];
      buffer_[1].get()[j++] = input_buffer.get()[i + 1];
    }
  }

  // Resample to match the system sample rate if needed.
  size_t system_hz = Engine::Get().GetAudioSampleRate();
  if (system_hz == 0 || system_hz == hz_)
    return;

  LOG << "Resampling from " << hz_ << " to " << system_hz;
  size_t resampled_num_samples = ((float)system_hz / (float)hz_) * num_samples_;

  r8b::CDSPResampler24 resampler(hz_, system_hz, num_samples_);

  for (int i = 0; i < num_channels_; ++i) {
    auto resampled_buffer_ = std::make_unique<float[]>(resampled_num_samples);
    resampler.oneshot(buffer_[i].get(), num_samples_, resampled_buffer_.get(),
                      resampled_num_samples);
    buffer_[i].swap(resampled_buffer_);
  }
  num_samples_ = resampled_num_samples;
}

}  // namespace eng
