#ifndef SOUND_H
#define SOUND_H

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

  bool DecodeNextFrame();

  // Buffer size per channel.
  size_t GetSize() const;

  const float* GetBuffer(int channel) const { return front_buffer_[channel].get(); }
  float* GetBuffer(int channel);

  bool IsValid() const { return !!front_buffer_[0]; }

  size_t num_samples() const { return num_samples_front_; }
  size_t num_channels() const { return num_channels_; }
  size_t hz() const { return hz_; }

 private:
  std::unique_ptr<float[]> buffer_[2];
  std::unique_ptr<float[]> front_buffer_[2];

  size_t num_samples_ = 0;
  size_t num_samples_front_ = 0;

  size_t num_channels_ = 0;
  size_t hz_ = 0;

  std::unique_ptr<mp3dec_ex_t> mp3_dec_;

  void Preprocess(std::unique_ptr<float[]> input_buffer);
};

}  // namespace eng

#endif  // SOUND_H
