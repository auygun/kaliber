#ifndef SOUND_H
#define SOUND_H

#include <stdint.h>
#include <atomic>
#include <memory>
#include <string>

typedef struct mp3dec_ex mp3dec_ex_t;

namespace r8b {
class CDSPResampler16;
}  // namespace r8b

namespace eng {

// Class for streaming and non-streaming sound assets. Loads and decodes mp3
// files. Resamples the decoded audio to match the system sample rate if
// necessary. Can be shared between multiple audio resources and played
// simultaneously.
//
// Streaming starts automatically for big files and it's done from memory. It
// loads the entire mp3 file and decodes small chunks on demand. Streaming sound
// cannot be shared between multiple audio resources.
class Sound {
 public:
  Sound();
  ~Sound();

  bool Load(const std::string& file_name);

  bool Stream(bool loop);

  void SwapBuffers();

  size_t IsStreamingInProgress() const;

  // Buffer size per channel.
  size_t GetSize() const;

  const float* GetBuffer(int channel) const {
    return front_buffer_[channel].get();
  }
  float* GetBuffer(int channel);

  bool IsValid() const { return !!front_buffer_[0]; }

  size_t GetNumSamples() const { return num_samples_front_; }

  size_t num_channels() const { return num_channels_; }
  size_t hz() const { return hz_; }

  bool is_streaming_sound() { return is_streaming_sound_; }

  bool eof() const { return eof_; }

 private:
  std::unique_ptr<float[]> back_buffer_[2];
  std::unique_ptr<float[]> front_buffer_[2];

  size_t num_samples_back_ = 0;
  size_t num_samples_front_ = 0;

  size_t num_channels_ = 0;
  size_t hz_ = 0;

  std::unique_ptr<char[]> encoded_data_;

  std::unique_ptr<mp3dec_ex_t> mp3_dec_;

  std::unique_ptr<r8b::CDSPResampler16> resampler_;

  bool eof_ = false;
  std::atomic<bool> streaming_in_progress_ = false;

  bool is_streaming_sound_ = false;

  bool StreamInternal(size_t num_samples, bool loop);

  void SwapBuffersInternal();

  void Preprocess(std::unique_ptr<float[]> input_buffer);
};

}  // namespace eng

#endif  // SOUND_H
