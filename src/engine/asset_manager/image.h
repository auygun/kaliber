#ifndef IMAGE_H
#define IMAGE_H

#include "../../base/vecmath.h"
#include "asset.h"
#include <stdint.h>

namespace engine {

class Image : public Asset{
 public:
  enum Format { kRGBA32, kDXT1, kDXT5, kETC1, kATC };

  Image();
  ~Image() override;

  bool Create(unsigned width, unsigned height);
  void Destroy();
  void Copy(const Image& image);

  bool Load(const char* file_name, bool convertPow2 = true);

  unsigned GetWidth() const { return width_; }
  unsigned GetHeight() const { return height_; }
  unsigned GetOriginalWidth() const { return original_width_; }
  unsigned GetOriginalHeight() const { return original_height_; }
  Format GetFormat() const { return format_; }
  bool IsCompressed() const { return format_ > kRGBA32; }

  unsigned GetSize() const;

  const uint8_t* GetBuffer() const { return buffer_; }
  uint8_t* GetBuffer() { return buffer_; }

  void Clear(const float* rgba);
  void Gradient();

  Vector2 GetUV() const { return uv_; }

 private:
  uint8_t* buffer_;
  unsigned width_;
  unsigned height_;
  unsigned original_width_;
  unsigned original_height_;
  Format format_;
  Vector2 uv_ = {1, 1};
};

}  // namespace engine

#endif  // IMAGE_H
