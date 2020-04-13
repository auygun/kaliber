#include "image.h"

#include <algorithm>
#include <cmath>

#include "../base/interpolation.h"
#include "../base/log.h"
#include "../base/misc.h"
#include "../third_party/texture_compressor/texture_compressor.h"
#include "engine.h"
#include "platform/asset_file.h"

// This 3rd party library is written in C and uses malloc, which means that we
// have to do the same.
#define STBI_NO_STDIO
#include "../third_party/stb/stb_image.h"

using namespace base;

namespace {

// Blend between two colors with equal weights.
uint32_t Mix2(uint32_t p0, uint32_t p1) {
  uint32_t r = (((p0 >> 0) & 0xff) + ((p1 >> 0) & 0xff)) / 2;
  uint32_t g = (((p0 >> 8) & 0xff) + ((p1 >> 8) & 0xff)) / 2;
  uint32_t b = (((p0 >> 16) & 0xff) + ((p1 >> 16) & 0xff)) / 2;
  uint32_t a = (((p0 >> 24) & 0xff) + ((p1 >> 24) & 0xff)) / 2;

  return (r << 0) | (g << 8) | (b << 16) | (a << 24);
}

// Blend between four colors with equal weights.
uint32_t Mix4(uint32_t p0, uint32_t p1, uint32_t p2, uint32_t p3) {
  uint32_t r = (((p0 >> 0) & 0xff) + ((p1 >> 0) & 0xff) + ((p2 >> 0) & 0xff) +
                ((p3 >> 0) & 0xff)) /
               4;
  uint32_t g = (((p0 >> 8) & 0xff) + ((p1 >> 8) & 0xff) + ((p2 >> 8) & 0xff) +
                ((p3 >> 8) & 0xff)) /
               4;
  uint32_t b = (((p0 >> 16) & 0xff) + ((p1 >> 16) & 0xff) +
                ((p2 >> 16) & 0xff) + ((p3 >> 16) & 0xff)) /
               4;
  uint32_t a = (((p0 >> 24) & 0xff) + ((p1 >> 24) & 0xff) +
                ((p2 >> 24) & 0xff) + ((p3 >> 24) & 0xff)) /
               4;

  return (r << 0) | (g << 8) | (b << 16) | (a << 24);
}

// Anisotropic blending of colors.
void MipNonUniform(void* dst, const void* src, size_t length) {
  const uint32_t* s = reinterpret_cast<const uint32_t*>(src);
  uint32_t* d = reinterpret_cast<uint32_t*>(dst);
  for (size_t y = 0; y < length; ++y) {
    *d++ = Mix2(s[0], s[1]);
    s += 2;
  }
}

}  // namespace

namespace eng {

Image::Image() = default;

Image::Image(const Image& other) {
  Copy(other);
}

Image::~Image() = default;

Image& Image::operator=(const Image& other) {
  Copy(other);
  return *this;
}

bool Image::Create(int w, int h) {
  width_ = w;
  height_ = h;

  buffer_.reset((uint8_t*)AlignedAlloc<16>(w * h * 4 * sizeof(uint8_t)));

  return true;
}

void Image::Copy(const Image& other) {
  if (other.buffer_) {
    int size = other.GetSize();
    buffer_.reset((uint8_t*)AlignedAlloc<16>(size));
    memcpy(buffer_.get(), other.buffer_.get(), size);
  }
  width_ = other.width_;
  height_ = other.height_;
  format_ = other.format_;
}

bool Image::CreateMip(const Image& other) {
  if (other.width_ <= 1 || other.height_ <= 1 || other.GetFormat() != kRGBA32)
    return false;

  // Reduce the dimensions.
  width_ = std::max(other.width_ >> 1, 1);
  height_ = std::max(other.height_ >> 1, 1);
  format_ = kRGBA32;
  buffer_.reset((uint8_t*)AlignedAlloc<16>(GetSize()));

  // If the width isn't perfectly divisable with two, then we end up skewing
  // the image because the source offset isn't updated properly.
  bool unaligned_width = other.width_ & 1;

  // Special case the non-uniform/anisotropic cases, eg 4:1 or 1:4 textures.
  // This is only an issue once we reach the highest mip levels where one
  // dimension is one pixel.
  if (other.width_ == 1) {
    // Interestingly the horizontal and vertical case becomes the same code,
    // it's only about which value to use as the run length that differs.
    MipNonUniform(buffer_.get(), other.buffer_.get(), height_);
  } else if (other.height_ == 1) {
    MipNonUniform(buffer_.get(), other.buffer_.get(), width_);
  } else {
    const uint32_t* s = reinterpret_cast<const uint32_t*>(other.buffer_.get());
    uint32_t* d = reinterpret_cast<uint32_t*>(buffer_.get());
    for (size_t y = 0; y < height_; ++y) {
      for (size_t x = 0; x < width_; ++x) {
        *d++ = Mix4(s[0], s[1], s[other.width_], s[other.width_ + 1]);
        s += 2;
      }
      if (unaligned_width)
        ++s;
      s += other.width_;
    }
  }

  return true;
}

bool Image::Load(const std::string& file_name) {
  size_t buffer_size = 0;
  auto file_buffer = AssetFile::ReadWholeFile(
      file_name.c_str(), Engine::Get().GetRootPath().c_str(), &buffer_size);
  if (!file_buffer) {
    LOG << "Failed to read file: " << file_name;
    return false;
  }

  int w, h, c;
  buffer_.reset((uint8_t*)stbi_load_from_memory(
      (const stbi_uc*)file_buffer.get(), buffer_size, &w, &h, &c, 0));
  if (!buffer_) {
    LOG << "Failed to load image file: " << file_name;
    return false;
  }

  uint8_t* converted_buffer = NULL;
  switch (c) {
    case 1:
      // LOG("Converting image from 1 to 4 channels.\n");
      // Assume it's an intensity, duplicate it to RGB and fill A with opaque.
      converted_buffer =
          (uint8_t*)AlignedAlloc<16>(w * h * 4 * sizeof(uint8_t));
      for (int i = 0; i < w * h; ++i) {
        converted_buffer[i * 4 + 0] = buffer_[i];
        converted_buffer[i * 4 + 1] = buffer_[i];
        converted_buffer[i * 4 + 2] = buffer_[i];
        converted_buffer[i * 4 + 3] = 255;
      }
      break;

    case 3:
      // LOG("Converting image from 3 to 4 channels.\n");
      // Add an opaque channel.
      converted_buffer =
          (uint8_t*)AlignedAlloc<16>(w * h * 4 * sizeof(uint8_t));
      for (int i = 0; i < w * h; ++i) {
        converted_buffer[i * 4 + 0] = buffer_[i * 3 + 0];
        converted_buffer[i * 4 + 1] = buffer_[i * 3 + 1];
        converted_buffer[i * 4 + 2] = buffer_[i * 3 + 2];
        converted_buffer[i * 4 + 3] = 255;
      }
      break;

    case 4:
      break;  // This is the wanted format.

    case 2:
    default:
      LOG << "Image had unsuitable number of color components: " << c << " "
          << file_name;
      buffer_.reset();
      return false;
  }

  if (converted_buffer)
    buffer_.reset(converted_buffer);

  width_ = w;
  height_ = h;

#if 0  // Fill the alpha channel with transparent gradient alpha for testing
  uint8_t* modifyBuf = buffer;
  for (int j = 0; j < height; ++j, modifyBuf += width * 4)
  {
    for (int i = 0; i < width; ++i)
    {
      float dist = sqrt(float(i*i + j*j));
      float alpha = (((dist > 0.0f ? dist : 0.0f) / sqrt((float)(width * width + height * height))) * 255.0f);
      modifyBuf[i * 4 + 3] = (unsigned char)alpha;
    }
  }
#endif

  return !!buffer_;
}

size_t Image::GetSize() const {
  switch (format_) {
    case kRGBA32:
      return width_ * height_ * 4;
    case kDXT1:
    case kATC:
      return ((width_ + 3) / 4) * ((height_ + 3) / 4) * 8;
    case kDXT5:
    case kATCIA:
      return ((width_ + 3) / 4) * ((height_ + 3) / 4) * 16;
    case kETC1:
      return (width_ * height_ * 4) / 8;
    default:
      return 0;
  }
}

void Image::ConvertToPow2() {
  int new_width = RoundUpToPow2(width_);
  int new_height = RoundUpToPow2(height_);
  if ((new_width != width_) || (new_height != height_)) {
    LOG << "Converting image from (" << width_ << ", " << height_ << ") to ("
        << new_width << ", " << new_height << ")";

    int bigger_size = new_width * new_height * 4 * sizeof(uint8_t);
    uint8_t* bigger_buffer = (uint8_t*)AlignedAlloc<16>(bigger_size);

    // Fill it with black.
    memset(bigger_buffer, 0, bigger_size);

    // Copy over the old bitmap.
#if 0
    // Centered in the new bitmap.
    int offset_x = (new_width - width_) / 2;
    int offset_y = (new_height - height_) / 2;
    for (int y = 0; y < height_; ++y)
      memcpy(bigger_buffer + (offset_x + (y + offset_y) * new_width) * 4,
              buffer_.get() + y * width_ * 4, width_ * 4);
#else
    for (int y = 0; y < height_; ++y)
      memcpy(bigger_buffer + (y * new_width) * 4,
             buffer_.get() + y * width_ * 4, width_ * 4);
#endif

    // Swap the buffers and dimensions.
    buffer_.reset(bigger_buffer);
    width_ = new_width;
    height_ = new_height;
  }
}

bool Image::Compress() {
  if (IsCompressed())
    return true;

  TextureCompressor* tc = Engine::Get().GetTextureCompressor(true);
  if (!tc)
    return false;

  switch (tc->format()) {
    case TextureCompressor::kFormatATC:
      format_ = kATC;
      break;
    case TextureCompressor::kFormatATCIA:
      format_ = kATCIA;
      break;
    case TextureCompressor::kFormatDXT1:
      format_ = kDXT1;
      break;
    case TextureCompressor::kFormatDXT5:
      format_ = kDXT5;
      break;
    case TextureCompressor::kFormatETC1:
      format_ = kETC1;
      break;
    default:
      return false;
  }

  LOG << "Compressing image. Format: " << format_;

  unsigned compressedSize = GetSize();
  uint8_t* compressedBuffer =
      (uint8_t*)AlignedAlloc<16>(compressedSize * sizeof(uint8_t));

  const uint8_t* src = buffer_.get();
  uint8_t* dst = compressedBuffer;

  tc->Compress(src, dst, width_, height_, TextureCompressor::kQualityHigh);

  buffer_.reset(compressedBuffer);
  return true;
}

uint8_t* Image::GetBuffer() {
  return buffer_.get();
}

void Image::Clear(Vector4 rgba) {
  // Quantize the color to target resolution.
  uint8_t r = (uint8_t)(rgba.x * 255.0f), g = (uint8_t)(rgba.y * 255.0f),
          b = (uint8_t)(rgba.z * 255.0f), a = (uint8_t)(rgba.w * 255.0f);

  // Fill out the first line manually.
  for (int w = 0; w < width_; ++w) {
    buffer_.get()[w * 4 + 0] = r;
    buffer_.get()[w * 4 + 1] = g;
    buffer_.get()[w * 4 + 2] = b;
    buffer_.get()[w * 4 + 3] = a;
  }

  // Copy the first line to the rest of them.
  for (int h = 1; h < height_; ++h)
    memcpy(buffer_.get() + h * width_ * 4, buffer_.get(), width_ * 4);
}

void Image::GradientH() {
  // Fill out the first line manually.
  for (int x = 0; x < width_; ++x) {
    uint8_t intensity = x > 255 ? 255 : x;
    buffer_.get()[x * 4 + 0] = intensity;
    buffer_.get()[x * 4 + 1] = intensity;
    buffer_.get()[x * 4 + 2] = intensity;
    buffer_.get()[x * 4 + 3] = 255;
  }

  // Copy the first line to the rest of them.
  for (int h = 1; h < height_; ++h)
    memcpy(buffer_.get() + h * width_ * 4, buffer_.get(), width_ * 4);
}

void Image::GradientV(const Vector4& c1, const Vector4& c2, int height) {
  // Fill each section with gradient.
  for (int h = 0; h < height_; ++h) {
    Vector4 c = Lerp(c1, c2, fmod(h, height) / (float)height);
    for (int x = 0; x < width_; ++x) {
      buffer_.get()[h * width_ * 4 + x * 4 + 0] = c.x * 255;
      buffer_.get()[h * width_ * 4 + x * 4 + 1] = c.y * 255;
      buffer_.get()[h * width_ * 4 + x * 4 + 2] = c.z * 255;
      buffer_.get()[h * width_ * 4 + x * 4 + 3] = 0;
    }
  }
}

}  // namespace eng
