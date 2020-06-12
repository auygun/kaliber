#include "sound.h"

#include "../base/log.h"
#include "../base/asset_file.h"
#define MINIMP3_ONLY_MP3
#define MINIMP3_ONLY_SIMD
#define MINIMP3_FLOAT_OUTPUT
#define MINIMP3_IMPLEMENTATION
#include "../third_party/minimp3/minimp3_ex.h"
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

  LOG << "MP3 decode: "
      << info.samples << " samples, "
      << info.channels << " channels, "
      << info.hz << " hz, "
      << "layer " << info.layer << ", "
      << "avg_bitrate_kbps " << info.avg_bitrate_kbps;

  buffer_.reset(info.buffer);
  num_samples_ = info.samples;
  num_channels_ = info.channels;
  hz_ = info.hz;

  return true;
}

size_t Sound::GetSize() const {
  return num_samples_ * sizeof(mp3d_sample_t);
}

float* Sound::GetBuffer() {
  if (IsImmutable()) {
    LOG << "Error: Asset is mutable. Failed to return writable buffer.";
    return nullptr;
  }

  return buffer_.get();
}

}  // namespace eng
