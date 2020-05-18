#ifndef IMAGE_H
#define IMAGE_H

#include "asset.h"
#include "../base/mem.h"
#include <stdint.h>

namespace eng {

class Image : public Asset{
 public:
  enum Format { kRGBA32, kDXT1, kDXT5, kETC1, kATC };

  Image();
  Image(const Image& other);
  ~Image() override;

  Image& operator=(const Image& other);

  bool Create(int width, int height);
  void Destroy();
  void Copy(const Image& other);

  bool Load(const char* file_name, bool convertPow2 = true);

  int GetWidth() const { return width_; }
  int GetHeight() const { return height_; }
  int GetOriginalWidth() const { return original_width_; }
  int GetOriginalHeight() const { return original_height_; }
  Format GetFormat() const { return format_; }
  bool IsCompressed() const { return format_ > kRGBA32; }

  int GetSize() const;

  const uint8_t* GetBuffer() const { return buffer_.get(); }
  uint8_t* GetBuffer();

  void Clear(const float* rgba);
  void Gradient();

 private:
  AlignedMem<uint8_t[]>::Scopped buffer_;
  int width_ = 0;
  int height_ = 0;
  int original_width_ = 0;
  int original_height_ = 0;
  Format format_ = kRGBA32;
};

}  // namespace eng

#endif  // IMAGE_H
