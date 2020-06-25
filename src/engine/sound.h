#ifndef SOUND_H
#define SOUND_H

#include <atomic>
#include <stdint.h>
#include <memory>
#include <string>

#include "asset.h"

typedef struct mp3dec_ex mp3dec_ex_t;

namespace eng {

class Sound : public Asset {
 public:
  Sound();
  ~Sound() override;

  bool Load(const std::string& file_name) override;

  bool Stream(bool loop);

  void SwapBuffers();

  size_t IsStreamingInProgress() const;

  // Buffer size per channel.
  size_t GetSize() const;

  const float* GetBuffer(int channel) const { return front_buffer_[channel].get(); }
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
  bool eof_ = false;
  std::atomic<bool> streaming_in_progress_ = false;

  bool is_streaming_sound_ = false;

  bool StreamInternal(size_t num_samples, bool loop);

  void SwapBuffersInternal();

  void Preprocess(std::unique_ptr<float[]> input_buffer);
};

}  // namespace eng

#endif  // SOUND_H
