#ifndef ENGINE_ASSET_IMAGE_H
#define ENGINE_ASSET_IMAGE_H

#include <stdint.h>
#include <string>

#include "base/mem.h"
#include "base/vecmath.h"
#include "engine/renderer/renderer_types.h"

namespace eng {

class Image {
 public:
  Image();
  Image(const Image& other);
  ~Image();

  Image& operator=(const Image& other);

  bool Create(int width, int height);
  void Copy(const Image& other);
  bool CreateMip(const Image& other, bool normalize);
  bool Load(const std::string& file_name);

  // Packs channels from two images into this image.
  // r_src, g_src, b_src, a_src: 1 for 'first' image, 2 for 'second' image.
  void Pack(const Image& first,
            const Image& second,
            int r_src,
            int g_src,
            int b_src,
            int a_src);

  bool Compress();

  void ConvertToPow2();

  int GetWidth() const { return width_; }
  int GetHeight() const { return height_; }

  ImageFormat GetFormat() const { return format_; }
  bool IsCompressed() const;

  size_t GetSize() const;

  const uint8_t* GetBuffer() const { return buffer_.get(); }
  uint8_t* GetBuffer();

  bool IsValid() const { return !!buffer_; }

  void Clear(base::Vector4f rgba);
  void GradientH();
  void GradientV(const base::Vector4f& c1,
                 const base::Vector4f& c2,
                 int height);

  void SRGB2Linear();
  void Linear2SRGB();

 private:
  base::AlignedMemPtr<uint8_t[]> buffer_;
  int width_ = 0;
  int height_ = 0;
  ImageFormat format_ = ImageFormat::kRGBA32;
};

}  // namespace eng

#endif  // ENGINE_ASSET_IMAGE_H
