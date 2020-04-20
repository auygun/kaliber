#include "image.h"
#include <stdlib.h>
#include <string.h>
#include <string>
#include "file.h"
#include "log.h"
#include "mem.h"
#include "misc.h"

// This 3rd party library is written in C and uses malloc, which means that we
// have to do the same.

#define STBI_NO_STDIO
#include "../third_party/stb/stb_image.h"

Image::Image()
    : buffer_(NULL), width_(0), height_(0), format_(kRGBA32), u_(1), v_(1) {}

Image::~Image() {
  Destroy();
}

bool Image::Create(unsigned w, unsigned h) {
  width_ = w;
  height_ = h;
  buffer_ = (uint8_t*)AlignedAlloc(w * h * 4 * sizeof(uint8_t));
  return !!buffer_;
}

void Image::Destroy() {
  AlignedFree(buffer_);
}

void Image::Copy(const Image& image) {
  if (image.buffer_) {
    unsigned size = image.GetSize();
    buffer_ = (uint8_t*)AlignedAlloc(size);
    memcpy(buffer_, image.buffer_, size);
  }
  width_ = image.width_;
  height_ = image.height_;
  format_ = image.format_;
  u_ = image.u_;
  v_ = image.v_;
}

bool Image::Load(const char* file_name, bool convert_pow2) {
  std::string fullPath = "images/";
  fullPath += file_name;

  unsigned fileSize = 0;
  char* fileBuffer = File::ReadWholeFile(fullPath.c_str(), &fileSize);
  if (!fileBuffer) {
    LOG("Failed to read file: %s\n", file_name);
    return false;
  }

  int w, h, c;
  buffer_ = (uint8_t*)stbi_load_from_memory((const stbi_uc*)fileBuffer,
                                            fileSize, &w, &h, &c, 0);
  if (!buffer_) {
    LOG("Failed to load image file: %s\n", file_name);
    return false;
  }

  uint8_t* convertedBuffer = NULL;
  switch (c) {
    case 1:
      // LOG("Converting image from 1 to 4 channels.\n");
      // Assume it's an intensity, duplicate it to RGB and fill A with opaque.
      convertedBuffer = (uint8_t*)AlignedAlloc(w * h * 4 * sizeof(uint8_t));
      for (unsigned i = 0; i < w * h; ++i) {
        convertedBuffer[i * 4 + 0] = buffer_[i];
        convertedBuffer[i * 4 + 1] = buffer_[i];
        convertedBuffer[i * 4 + 2] = buffer_[i];
        convertedBuffer[i * 4 + 3] = 255;
      }
      break;

    case 3:
      // LOG("Converting image from 3 to 4 channels.\n");
      // Add an opaque channel.
      convertedBuffer = (uint8_t*)AlignedAlloc(w * h * 4 * sizeof(uint8_t));
      for (unsigned i = 0; i < w * h; ++i) {
        convertedBuffer[i * 4 + 0] = buffer_[i * 3 + 0];
        convertedBuffer[i * 4 + 1] = buffer_[i * 3 + 1];
        convertedBuffer[i * 4 + 2] = buffer_[i * 3 + 2];
        convertedBuffer[i * 4 + 3] = 255;
      }
      break;

    case 4:
      break;  // This is the wanted format.

    case 2:
    default:
      LOG("Image had unsuitable number of color components: %d %s\n", c,
          file_name);
      return false;
  }

  if (convertedBuffer) {
    AlignedFree(buffer_);
    buffer_ = convertedBuffer;
  }

  width_ = (unsigned)w;
  height_ = (unsigned)h;

  // Create a bigger canvas if needed to satisfy the pow2 dimension requirement.
  if (convert_pow2) {
    unsigned newWidth = RoundUpToPow2(width_);
    unsigned newHeight = RoundUpToPow2(height_);
    if ((newWidth != width_) || (newHeight != height_)) {
      LOG("Converting loaded image from (%d, %d) to (%d, %d)\n", width_,
          height_, newWidth, newHeight);

      unsigned biggerSize = newWidth * newHeight * 4 * sizeof(uint8_t);
      uint8_t* biggerBuffer = (uint8_t*)AlignedAlloc(biggerSize);

      // Fill it with black.
      memset(biggerBuffer, 0, biggerSize);

      // Copy over the old bitmap.
      // Centered in the new bitmap.
      int offsetX = (newWidth - width_) / 2,
          offsetY = (newHeight - height_) / 2;
      for (unsigned y = 0; y < height_; ++y)
        memcpy(biggerBuffer + (offsetX + (y + offsetY) * newWidth) * 4,
               buffer_ + y * width_ * 4, width_ * 4);

      // Store the texture coordinate scaling.
      u_ = width_ / (float)newWidth;
      v_ = height_ / (float)newHeight;

      // Swap the buffers and dimensions.
      AlignedFree(buffer_);
      buffer_ = biggerBuffer;
      width_ = newWidth;
      height_ = newHeight;
    }
  }

#if 0  // Fill the alpha channel with transparent gradient alpha for testing
  uint8_t   *modifyBuf = buffer;
  for( int j = 0; j < height; ++j, modifyBuf += width * 4)
  {
    for( int i = 0; i < width; ++i)
    {
      float dist = sqrt(float(i*i + j*j));
      float alpha = (((dist > 0.0f ? dist : 0.0f) / sqrt((float)(width*width + height*height))) * 255.0f);
      modifyBuf[i * 4 + 3] = (unsigned char)alpha;
    }
  }
#endif

  delete[] fileBuffer;
  return !!buffer_;
}

unsigned Image::GetSize() const {
  switch (format_) {
    case kRGBA32:
      return width_ * height_ * 4;
    case kDXT1:
      return ((width_ + 3) / 4) * ((height_ + 3) / 4) * 8;
    case kDXT5:
      return ((width_ + 3) / 4) * ((height_ + 3) / 4) * 16;
    case kATC:
      return ((width_ + 3) / 4) * ((height_ + 3) / 4) * 16;
    case kETC1:
      return (width_ * height_ * 4) / 8;
    default:
      return 0;
  }
}

void Image::Clear(const float* rgba) {
  // Quantize the color to target resolution.
  uint8_t r = (uint8_t)(rgba[0] * 255.0f), g = (uint8_t)(rgba[1] * 255.0f),
          b = (uint8_t)(rgba[2] * 255.0f), a = (uint8_t)(rgba[3] * 255.0f);

  // Fill out the first line manually.
  for (unsigned w = 0; w < width_; ++w) {
    buffer_[w * 4 + 0] = r;
    buffer_[w * 4 + 1] = g;
    buffer_[w * 4 + 2] = b;
    buffer_[w * 4 + 3] = a;
  }

  // Copy the first line to the rest of them.
  for (unsigned h = 1; h < height_; ++h)
    memcpy(buffer_ + h * width_ * 4, buffer_, width_ * 4);
}

void Image::Gradient() {
  // Fill out the first line manually.
  for (unsigned x = 0; x < width_; ++x) {
    uint8_t intensity = x > 255 ? 255 : x;
    buffer_[x * 4 + 0] = intensity;
    buffer_[x * 4 + 1] = intensity;
    buffer_[x * 4 + 2] = intensity;
    buffer_[x * 4 + 3] = 255;
  }

  // Copy the first line to the rest of them.
  for (unsigned h = 1; h < height_; ++h)
    memcpy(buffer_ + h * width_ * 4, buffer_, width_ * 4);
}
