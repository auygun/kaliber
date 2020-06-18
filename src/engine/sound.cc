#include "sound.h"

#include <cassert>

#include "../base/log.h"
#include "../base/asset_file.h"
#define MINIMP3_ONLY_MP3
#define MINIMP3_ONLY_SIMD
#define MINIMP3_FLOAT_OUTPUT
#define MINIMP3_IMPLEMENTATION
#include "../third_party/minimp3/minimp3_ex.h"
#include "../third_party/r8b/CDSPResampler.h"
#include "engine.h"

using namespace base;

namespace eng {

Sound::Sound() {
}

Sound::~Sound() {
}

bool Sound::Load(const std::string& file_name) {
  if (IsImmutable()) {
    LOG << "Error: Asset is mutable. Failed to load.";
    return false;
  }

  SetName(file_name);

  size_t buffer_size = 0;
  auto file_buffer = AssetFile::ReadWholeFile(
      file_name.c_str(), Engine::Get().GetRootPath().c_str(), &buffer_size,
      false);
  if (!file_buffer) {
    LOG << "Failed to read file: " << file_name;
    return false;
  }

  mp3dec_t mp3d;
  mp3dec_file_info_t info;
  int err = mp3dec_load_buf(&mp3d,
                            reinterpret_cast<uint8_t*>(file_buffer.get()),
                            buffer_size, &info, nullptr, nullptr);
  if (err) {
    LOG << "Failed to decode file: " << file_name << " error: " << err ;
    return false;
  }

  LOG << "Decoded " << GetName() << ". "
      << info.samples << " samples, "
      << info.channels << " channels, "
      << info.hz << " hz, "
      << "layer " << info.layer << ", "
      << "avg_bitrate_kbps " << info.avg_bitrate_kbps;

  num_samples_ = info.samples / info.channels;
  num_channels_ = info.channels;
  hz_ = info.hz;

  assert(num_channels_ > 0 && num_channels_ <= 2);

  std::unique_ptr<float[]> input_buffer;
  input_buffer.reset(info.buffer);

  Preprocess(std::move(input_buffer));

  return true;
}

size_t Sound::GetSize() const {
  return num_samples_ * sizeof(mp3d_sample_t);
}

float* Sound::GetBuffer(int channel) {
  if (IsImmutable()) {
    LOG << "Error: Asset is mutable. Failed to return writable buffer.";
    return nullptr;
  }

  return buffer_[channel].get();
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
  if (system_hz == hz_)
    return;

  LOG << "Resampling from " << hz_ << " to " << system_hz;
  size_t resampled_num_samples = ((float)system_hz / (float)hz_) *
                                   num_samples_;

	r8b::CDSPResampler24 resampler(hz_, system_hz, num_samples_);

  for (int i = 0; i < num_channels_; ++i) {
    auto resampled_buffer_ = std::make_unique<float[]>(resampled_num_samples);
    resampler.oneshot(buffer_[i].get(),
                      num_samples_,
                      resampled_buffer_.get(),
                      resampled_num_samples);
    buffer_[i].swap(resampled_buffer_);
  }
  num_samples_ = resampled_num_samples;
}

}  // namespace eng
