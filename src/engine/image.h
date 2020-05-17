#ifndef IMAGE_H
#define IMAGE_H

#include "asset.h"
#include <stdint.h>

namespace eng {

class Image : public Asset{
 public:
  enum Format { kRGBA32, kDXT1, kDXT5, kETC1, kATC };

  Image();
  ~Image() override;

  bool Create(unsigned width, unsigned height);
  void Destroy();
  void Copy(const Image& image);

  bool Load(const char* file_name, bool convertPow2 = true);

  int GetWidth() const { return width_; }
  int GetHeight() const { return height_; }
  int GetOriginalWidth() const { return original_width_; }
  int GetOriginalHeight() const { return original_height_; }
  Format GetFormat() const { return format_; }
  bool IsCompressed() const { return format_ > kRGBA32; }

  int GetSize() const;

  const uint8_t* GetBuffer() const { return buffer_; }
  uint8_t* GetBuffer();

  void Clear(const float* rgba);
  void Gradient();

 private:
  uint8_t* buffer_;
  int width_;
  int height_;
  int original_width_;
  int original_height_;
  Format format_;
};

}  // namespace eng

#endif  // IMAGE_H
