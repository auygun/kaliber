#include "engine/sound.h"

#include <array>

#include "base/log.h"
#define MINIMP3_ONLY_MP3
#define MINIMP3_ONLY_SIMD
#define MINIMP3_FLOAT_OUTPUT
#define MINIMP3_NO_STDIO
#define MINIMP3_IMPLEMENTATION
#include "engine/engine.h"
#include "engine/platform/asset_file.h"
#include "third_party/minimp3/minimp3_ex.h"

using namespace base;

namespace eng {

constexpr size_t kMaxSamplesPerChunk = MINIMP3_MAX_SAMPLES_PER_FRAME;

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

  LOG << (stream ? "Streaming " : "Loaded ") << file_name << ". "
      << mp3_dec_->samples << " samples, " << mp3_dec_->info.channels
      << " channels, " << mp3_dec_->info.hz << " hz, "
      << "layer " << mp3_dec_->info.layer << ", "
      << "avg_bitrate_kbps " << mp3_dec_->info.bitrate_kbps;

  SetAudioConfig(mp3_dec_->info.channels, mp3_dec_->info.hz);

  samples_per_channel_ = 0;
  eos_ = false;

  DCHECK(mp3_dec_->info.channels > 0 && mp3_dec_->info.channels <= 2);

  if (stream) {
    // Fill up the buffer.
    StreamInternal(kMaxSamplesPerChunk * mp3_dec_->info.channels, false);
    SwapBuffers();
    StreamInternal(kMaxSamplesPerChunk * mp3_dec_->info.channels, false);
    // No need to stream if sample fits into the buffer.
    if (eos_)
      stream = false;
  } else {
    // Decode entire file.
    StreamInternal(mp3_dec_->samples, false);
    SwapBuffers();
    eos_ = true;
  }

  if (!stream) {
    // We are done with decoding.
    encoded_data_.reset();
    mp3dec_ex_close(mp3_dec_.get());
    mp3_dec_.reset();
  }

  read_pos_ = 0;

  return true;
}

void Sound::Stream(bool loop) {
  DCHECK(mp3_dec_);
  StreamInternal(kMaxSamplesPerChunk * mp3_dec_->info.channels, loop);
}

void Sound::SwapBuffers() {
  FromInterleaved(std::move(interleaved_data_), samples_per_channel_);
  samples_per_channel_ = 0;
}

void Sound::ResetStream() {
  if (mp3_dec_ && read_pos_ != 0) {
    // Seek to 0 and stream.
    mp3dec_ex_seek(mp3_dec_.get(), 0);
    eos_ = false;
    StreamInternal(kMaxSamplesPerChunk * mp3_dec_->info.channels, false);
    SwapBuffers();
    StreamInternal(kMaxSamplesPerChunk * mp3_dec_->info.channels, false);
    read_pos_ = 0;
  }
}

void Sound::StreamInternal(size_t num_samples, bool loop) {
  auto buffer = std::make_unique<float[]>(num_samples);
  size_t samples_read_per_channel = 0;

  read_pos_ = mp3_dec_->cur_sample;

  for (;;) {
    size_t samples_read =
        mp3dec_ex_read(mp3_dec_.get(), buffer.get(), num_samples);
    if (samples_read != num_samples && mp3_dec_->last_error) {
      LOG << "mp3 decode error: " << mp3_dec_->last_error;
      break;
    }

    if (samples_read == 0 && loop) {
      mp3dec_ex_seek(mp3_dec_.get(), 0);
      loop = false;
      continue;
    }

    samples_read_per_channel = samples_read / mp3_dec_->info.channels;
    break;
  }

  if (samples_read_per_channel > 0) {
    interleaved_data_ = std::move(buffer);
    samples_per_channel_ = samples_read_per_channel;
  } else {
    samples_per_channel_ = 0;
    eos_ = true;
  }
}

}  // namespace eng
