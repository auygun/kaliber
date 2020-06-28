#ifndef IMAGE_H
#define IMAGE_H

#include <stdint.h>
#include <string>

#include "../base/mem.h"
#include "../base/vecmath.h"

namespace eng {

class Image {
 public:
  enum Format { kRGBA32, kDXT1, kDXT5, kETC1, kATC, kATCIA };

  Image();
  Image(const Image& other);
  ~Image();

  Image& operator=(const Image& other);

  bool Create(int width, int height);
  void Copy(const Image& other);
  bool CreateMip(const Image& other);
  bool Load(const std::string& file_name);

  bool Compress();

  void ConvertToPow2();

  int GetWidth() const { return width_; }
  int GetHeight() const { return height_; }

  Format GetFormat() const { return format_; }
  bool IsCompressed() const { return format_ > kRGBA32; }

  size_t GetSize() const;

  const uint8_t* GetBuffer() const { return buffer_.get(); }
  uint8_t* GetBuffer();

  bool IsValid() const { return !!buffer_; }

  void Clear(base::Vector4 rgba);
  void GradientH();
  void GradientV(const base::Vector4& c1, const base::Vector4& c2, int height);

 private:
  base::AlignedMem<uint8_t[]>::ScoppedPtr buffer_;
  int width_ = 0;
  int height_ = 0;
  Format format_ = kRGBA32;

  std::string name_;
};

}  // namespace eng

#endif  // IMAGE_H
