#ifndef SOUND_H
#define SOUND_H

#include <memory>
#include <stdint.h>
#include <string>

#include "asset.h"

namespace eng {

class Sound : public Asset {
 public:
  Sound();
  ~Sound() override;

  bool Load(const std::string& file_name) override;

  size_t GetSize() const;

  const float* GetBuffer() const { return buffer_.get(); }
  float* GetBuffer();

  bool IsValid() const { return !!buffer_; }

  size_t num_samples() const { return num_samples_; }
  size_t num_channels() const { return num_samples_; }
  size_t hz() const { return hz_; }

 private:
  std::unique_ptr<float[]> buffer_;
  size_t num_samples_ = 0;
  int num_channels_ = 0;
  int hz_ = 0;
};

}  // namespace eng

#endif  // SOUND_H
