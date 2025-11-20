#include "engine/asset/image.h"

#include <algorithm>
#include <cmath>
#include <cstring>

#include "base/interpolation.h"
#include "base/log.h"
#include "base/mem.h"
#include "base/misc.h"
#include "engine/engine.h"
#include "engine/platform/asset_file.h"
#include "third_party/texture_compressor/texture_compressor.h"

// Use aligned memory for SIMD in texture compressor.
#define STBI_NO_STDIO
#include "third_party/stb/stb_image.h"

using namespace base;

namespace {

uint8_t Average4Uint8(uint8_t c0, uint8_t c1, uint8_t c2, uint8_t c3) {
  return static_cast<uint8_t>((c0 + c1 + c2 + c3 + 2) >> 2);
}

void NormalizeUint8(uint8_t* rgb) {
  // Convert to float, scale and bias to [−1,1] before normalization.
  Vector3f n(rgb[0] / 255.0, rgb[1] / 255.0, rgb[2] / 255.0);
  n *= 2.0;
  n -= Vector3f(1, 1, 1);
  n.Normalize();
  // Revers scale and bias, convert to uint8.
  n += Vector3f(1, 1, 1);
  n *= 0.5;
  n *= 255;
  rgb[0] = std::clamp(int(n.x), 0, 255);
  rgb[1] = std::clamp(int(n.y), 0, 255);
  rgb[2] = std::clamp(int(n.z), 0, 255);
}

template <typename CT,
          int CC,
          bool normalize,
          CT (*average_func)(CT, CT, CT, CT),
          void (*normalize_func)(CT*)>
void GenerateMip(const CT* src,
                 CT* dst,
                 uint32_t src_width,
                 uint32_t src_height,
                 uint32_t dst_width,
                 uint32_t dst_height) {
  int right_step = (src_width == 1) ? 0 : CC;
  int down_step = (src_height == 1) ? 0 : (src_width * CC);

  for (uint32_t i = 0; i < dst_height; i++) {
    const CT* rup_ptr = &src[i * 2 * down_step];
    const CT* rdown_ptr = rup_ptr + down_step;
    CT* dst_ptr = &dst[i * dst_width * CC];
    uint32_t count = dst_width;

    while (count) {
      count--;
      for (int j = 0; j < CC; j++)
        dst_ptr[j] = average_func(rup_ptr[j], rup_ptr[j + right_step],
                                  rdown_ptr[j], rdown_ptr[j + right_step]);

      if (normalize)
        normalize_func(dst_ptr);

      dst_ptr += CC;
      rup_ptr += right_step * 2;
      rdown_ptr += right_step * 2;
    }
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

  buffer_.reset((uint8_t*)AlignedAlloc(w * h * 4 * sizeof(uint8_t), 16));

  return true;
}

void Image::Copy(const Image& other) {
  if (other.buffer_) {
    int size = other.GetSize();
    buffer_.reset((uint8_t*)AlignedAlloc(size, 16));
    std::memcpy(buffer_.get(), other.buffer_.get(), size);
  }
  width_ = other.width_;
  height_ = other.height_;
  format_ = other.format_;
}

bool Image::CreateMip(const Image& other, bool normalize) {
  if (other.width_ <= 1 || other.height_ <= 1 ||
      other.GetFormat() != ImageFormat::kRGBA32)
    return false;

  // Reduce the dimensions.
  width_ = std::max(other.width_ >> 1, 1);
  height_ = std::max(other.height_ >> 1, 1);
  format_ = ImageFormat::kRGBA32;
  buffer_.reset((uint8_t*)AlignedAlloc(GetSize(), 16));

  if (normalize) {
    GenerateMip<uint8_t, 4, true, Average4Uint8, NormalizeUint8>(
        other.buffer_.get(), buffer_.get(), other.width_, other.height_, width_,
        height_);
  } else {
    GenerateMip<uint8_t, 4, false, Average4Uint8, NormalizeUint8>(
        other.buffer_.get(), buffer_.get(), other.width_, other.height_, width_,
        height_);
  }

  return true;
}

bool Image::Load(const std::string& file_name) {
  size_t buffer_size = 0;
  auto file_buffer = AssetFile::ReadWholeFile(
      file_name.c_str(), Engine::Get().GetRootPath().c_str(), &buffer_size);
  if (!file_buffer) {
    LOG(0) << "Failed to read file: " << file_name;
    return false;
  }

  int w, h, c;
  buffer_.reset((uint8_t*)stbi_load_from_memory(
      (const stbi_uc*)file_buffer.get(), buffer_size, &w, &h, &c, 0));
  if (!buffer_) {
    LOG(0) << "Failed to load image file: " << file_name;
    return false;
  }

  DLOG(0) << "Loaded " << file_name << ". number of color components: " << c;

  uint8_t* converted_buffer = NULL;
  switch (c) {
    case 1:
      // LOG(0)("Converting image from 1 to 4 channels.\n");
      // Assume it's an intensity, duplicate it to RGB and fill A with opaque.
      converted_buffer =
          (uint8_t*)AlignedAlloc(w * h * 4 * sizeof(uint8_t), 16);
      for (int i = 0; i < w * h; ++i) {
        converted_buffer[i * 4 + 0] = buffer_[i];
        converted_buffer[i * 4 + 1] = buffer_[i];
        converted_buffer[i * 4 + 2] = buffer_[i];
        converted_buffer[i * 4 + 3] = 255;
      }
      break;

    case 3:
      // LOG(0)("Converting image from 3 to 4 channels.\n");
      // Add an opaque channel.
      converted_buffer =
          (uint8_t*)AlignedAlloc(w * h * 4 * sizeof(uint8_t), 16);
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
      LOG(0) << "Image had unsuitable number of color components: " << c << " "
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

void Image::Pack(const Image& first,
                 const Image& second,
                 int r_src,
                 int g_src,
                 int b_src,
                 int a_src) {
  DCHECK(first.width_ != 0 && first.height_ != 0);
  DCHECK(first.width_ == second.width_ && first.height_ == second.height_);

  width_ = first.width_;
  height_ = first.height_;
  buffer_.reset(
      (uint8_t*)AlignedAlloc(width_ * height_ * 4 * sizeof(uint8_t), 16));

  uint8_t* r_src_data = r_src == 1 ? first.buffer_.get() : second.buffer_.get();
  uint8_t* g_src_data = g_src == 1 ? first.buffer_.get() : second.buffer_.get();
  uint8_t* b_src_data = b_src == 1 ? first.buffer_.get() : second.buffer_.get();
  uint8_t* a_src_data = a_src == 1 ? first.buffer_.get() : second.buffer_.get();

  for (int i = 0; i < width_ * height_; ++i) {
    buffer_[i * 4 + 0] = r_src_data[i * 4 + 0];
    buffer_[i * 4 + 1] = g_src_data[i * 4 + 1];
    buffer_[i * 4 + 2] = b_src_data[i * 4 + 2];
    buffer_[i * 4 + 3] = a_src_data[i * 4 + 3];
  }
}

bool Image::IsCompressed() const {
  return IsCompressedFormat(format_);
}

size_t Image::GetSize() const {
  return GetImageSize(width_, height_, format_);
}

void Image::ConvertToPow2() {
  int new_width = RoundUpToPow2(width_);
  int new_height = RoundUpToPow2(height_);
  if ((new_width != width_) || (new_height != height_)) {
    DLOG(0) << "Converting image from (" << width_ << ", " << height_
            << ") to (" << new_width << ", " << new_height << ")";

    int bigger_size = new_width * new_height * 4 * sizeof(uint8_t);
    uint8_t* bigger_buffer = (uint8_t*)AlignedAlloc(bigger_size, 16);

    // Fill it with black.
    memset(bigger_buffer, 0, bigger_size);

    // Copy over the old bitmap.
#if 0
    // Centered in the new bitmap.
    int offset_x = (new_width - width_) / 2;
    int offset_y = (new_height - height_) / 2;
    for (int y = 0; y < height_; ++y)
      std::memcpy(bigger_buffer + (offset_x + (y + offset_y) * new_width) * 4,
              buffer_.get() + y * width_ * 4, width_ * 4);
#else
    for (int y = 0; y < height_; ++y)
      std::memcpy(bigger_buffer + (y * new_width) * 4,
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
      format_ = ImageFormat::kATC;
      break;
    case TextureCompressor::kFormatATCIA:
      format_ = ImageFormat::kATCIA;
      break;
    case TextureCompressor::kFormatDXT1:
      format_ = ImageFormat::kDXT1;
      break;
    case TextureCompressor::kFormatDXT5:
      format_ = ImageFormat::kDXT5;
      break;
    case TextureCompressor::kFormatETC1:
      format_ = ImageFormat::kETC1;
      break;
    default:
      return false;
  }

  DLOG(0) << "Compressing image. Format: " << ImageFormatToString(format_);

  unsigned compressedSize = GetSize();
  uint8_t* compressedBuffer =
      (uint8_t*)AlignedAlloc(compressedSize * sizeof(uint8_t), 16);

  const uint8_t* src = buffer_.get();
  uint8_t* dst = compressedBuffer;

  tc->Compress(src, dst, width_, height_, TextureCompressor::kQualityHigh);

  buffer_.reset(compressedBuffer);
  return true;
}

uint8_t* Image::GetBuffer() {
  return buffer_.get();
}

void Image::Clear(Vector4f rgba) {
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
    std::memcpy(buffer_.get() + h * width_ * 4, buffer_.get(), width_ * 4);
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
    std::memcpy(buffer_.get() + h * width_ * 4, buffer_.get(), width_ * 4);
}

void Image::GradientV(const Vector4f& c1, const Vector4f& c2, int height) {
  // Fill each section with gradient.
  for (int h = 0; h < height_; ++h) {
    Vector4f c = Lerp(c1, c2, fmod(h, height) / (float)height);
    for (int x = 0; x < width_; ++x) {
      buffer_.get()[h * width_ * 4 + x * 4 + 0] = c.x * 255;
      buffer_.get()[h * width_ * 4 + x * 4 + 1] = c.y * 255;
      buffer_.get()[h * width_ * 4 + x * 4 + 2] = c.z * 255;
      buffer_.get()[h * width_ * 4 + x * 4 + 3] = 0;
    }
  }
}

void Image::SRGB2Linear() {
  DCHECK(buffer_);

  static const uint8_t srgb2lin[256] = {
      0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   1,   1,
      1,   1,   1,   1,   1,   1,   1,   2,   2,   2,   2,   2,   2,   2,   3,
      3,   3,   3,   3,   4,   4,   4,   4,   4,   5,   5,   5,   5,   6,   6,
      6,   6,   7,   7,   7,   8,   8,   8,   9,   9,   9,   10,  10,  10,  11,
      11,  11,  12,  12,  13,  13,  13,  14,  14,  15,  15,  16,  16,  16,  17,
      17,  18,  18,  19,  19,  20,  20,  21,  22,  22,  23,  23,  24,  24,  25,
      26,  26,  27,  27,  28,  29,  29,  30,  31,  31,  32,  33,  33,  34,  35,
      36,  36,  37,  38,  38,  39,  40,  41,  42,  42,  43,  44,  45,  46,  47,
      47,  48,  49,  50,  51,  52,  53,  54,  55,  55,  56,  57,  58,  59,  60,
      61,  62,  63,  64,  65,  66,  67,  68,  70,  71,  72,  73,  74,  75,  76,
      77,  78,  80,  81,  82,  83,  84,  85,  87,  88,  89,  90,  92,  93,  94,
      95,  97,  98,  99,  101, 102, 103, 105, 106, 107, 109, 110, 112, 113, 114,
      116, 117, 119, 120, 122, 123, 125, 126, 128, 129, 131, 132, 134, 135, 137,
      139, 140, 142, 144, 145, 147, 148, 150, 152, 153, 155, 157, 159, 160, 162,
      164, 166, 167, 169, 171, 173, 175, 176, 178, 180, 182, 184, 186, 188, 190,
      192, 193, 195, 197, 199, 201, 203, 205, 207, 209, 211, 213, 215, 218, 220,
      222, 224, 226, 228, 230, 232, 235, 237, 239, 241, 243, 245, 248, 250, 252,
      255};

  if (format_ == ImageFormat::kRGBA32) {
    int len = GetSize() / 4;
    uint8_t* data_ptr = buffer_.get();

    for (int i = 0; i < len; i++) {
      data_ptr[(i << 2) + 0] = srgb2lin[data_ptr[(i << 2) + 0]];
      data_ptr[(i << 2) + 1] = srgb2lin[data_ptr[(i << 2) + 1]];
      data_ptr[(i << 2) + 2] = srgb2lin[data_ptr[(i << 2) + 2]];
    }
  }
#if 0
  else if (format_ == ImageFormat::kRGB8) {
    int len = GetSize() / 3;
    uint8_t* data_ptr = buffer_.get();

    for (int i = 0; i < len; i++) {
      data_ptr[(i * 3) + 0] = srgb2lin[data_ptr[(i * 3) + 0]];
      data_ptr[(i * 3) + 1] = srgb2lin[data_ptr[(i * 3) + 1]];
      data_ptr[(i * 3) + 2] = srgb2lin[data_ptr[(i * 3) + 2]];
    }
  }
#endif
  else {
    NOTREACHED() << "invalid image format: " << static_cast<int>(format_);
  }
}

void Image::Linear2SRGB() {
  DCHECK(buffer_);

  static const uint8_t lin2srgb[256] = {
      0,   12,  21,  28,  33,  38,  42,  46,  49,  52,  55,  58,  61,  63,  66,
      68,  70,  73,  75,  77,  79,  81,  82,  84,  86,  88,  89,  91,  93,  94,
      96,  97,  99,  100, 102, 103, 104, 106, 107, 109, 110, 111, 112, 114, 115,
      116, 117, 118, 120, 121, 122, 123, 124, 125, 126, 127, 129, 130, 131, 132,
      133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 142, 143, 144, 145, 146,
      147, 148, 149, 150, 151, 151, 152, 153, 154, 155, 156, 157, 157, 158, 159,
      160, 161, 161, 162, 163, 164, 165, 165, 166, 167, 168, 168, 169, 170, 171,
      171, 172, 173, 174, 174, 175, 176, 176, 177, 178, 179, 179, 180, 181, 181,
      182, 183, 183, 184, 185, 185, 186, 187, 187, 188, 189, 189, 190, 191, 191,
      192, 193, 193, 194, 194, 195, 196, 196, 197, 197, 198, 199, 199, 200, 201,
      201, 202, 202, 203, 204, 204, 205, 205, 206, 206, 207, 208, 208, 209, 209,
      210, 210, 211, 212, 212, 213, 213, 214, 214, 215, 215, 216, 217, 217, 218,
      218, 219, 219, 220, 220, 221, 221, 222, 222, 223, 223, 224, 224, 225, 226,
      226, 227, 227, 228, 228, 229, 229, 230, 230, 231, 231, 232, 232, 233, 233,
      234, 234, 235, 235, 236, 236, 237, 237, 237, 238, 238, 239, 239, 240, 240,
      241, 241, 242, 242, 243, 243, 244, 244, 245, 245, 245, 246, 246, 247, 247,
      248, 248, 249, 249, 250, 250, 251, 251, 251, 252, 252, 253, 253, 254, 254,
      255};

  if (format_ == ImageFormat::kRGBA32) {
    int len = GetSize() / 4;
    uint8_t* data_ptr = buffer_.get();

    for (int i = 0; i < len; i++) {
      data_ptr[(i << 2) + 0] = lin2srgb[data_ptr[(i << 2) + 0]];
      data_ptr[(i << 2) + 1] = lin2srgb[data_ptr[(i << 2) + 1]];
      data_ptr[(i << 2) + 2] = lin2srgb[data_ptr[(i << 2) + 2]];
    }
  }
  // else if (format == FORMAT_RGB8) {
  //   int len = data.size() / 3;
  //   uint8_t* data_ptr = data.ptrw();

  //   for (int i = 0; i < len; i++) {
  //     data_ptr[(i * 3) + 0] = lin2srgb[data_ptr[(i * 3) + 0]];
  //     data_ptr[(i * 3) + 1] = lin2srgb[data_ptr[(i * 3) + 1]];
  //     data_ptr[(i * 3) + 2] = lin2srgb[data_ptr[(i * 3) + 2]];
  //   }
  // }
  else {
    NOTREACHED() << "invalid image format: " << static_cast<int>(format_);
  }
}

}  // namespace eng
