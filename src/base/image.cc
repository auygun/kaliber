#include "file.h"
#include "log.h"
#include "mem.h"
#include "misc.h"
#include "image.h"
#include <string>
#include <string.h>
#include <stdlib.h>

// This 3rd party library is written in C and uses malloc, which means that we
// have to do the same.

#define STBI_NO_STDIO
#include "../third_party/stb/stb_image.h"

Image::Image()
  : buffer(NULL)
  , width(0)
  , height(0)
  , format(kRGBA32)
  , u(1)
  , v(1) {
}

Image::~Image() {
  Destroy();
}

bool Image::Create(unsigned w, unsigned h) {
  width = w;
  height = h;
  buffer = (uint8_t *)AlignedAlloc(w * h * 4 * sizeof(uint8_t));
  return !!buffer;
}

void Image::Destroy() {
  AlignedFree(buffer);
}

void Image::Copy(const Image &image) {
  if (image.buffer) {
    unsigned size = image.GetSize();
    buffer = (uint8_t *)AlignedAlloc(size);
    memcpy(buffer, image.buffer, size);
  }
  width = image.width;
  height = image.height;
  format = image.format;
  u = image.u;
  v = image.v;
}

bool Image::Load(const char *fileName, bool convertPow2) {
  std::string fullPath = "images/";
  fullPath += fileName;

  unsigned fileSize = 0;
  char *fileBuffer = File::ReadWholeFile(fullPath.c_str(), &fileSize);
  if (!fileBuffer) {
    LOG("Failed to read file: %s\n", fileName);
    return false;
  }

  int w, h, c;
  buffer = (uint8_t *)stbi_load_from_memory((const stbi_uc *)fileBuffer, fileSize, &w, &h, &c, 0);
  if (!buffer) {
    LOG("Failed to load image file: %s\n", fileName);
    return false;
  }

  uint8_t *convertedBuffer = NULL;
  switch (c) {
  case 1:
    //LOG("Converting image from 1 to 4 channels.\n");
    // Assume it's an intensity, duplicate it to RGB and fill A with opaque.
    convertedBuffer = (uint8_t *)AlignedAlloc(w * h * 4 * sizeof(uint8_t));
    for (unsigned i = 0; i < w * h; ++i) {
      convertedBuffer[i * 4 + 0] = buffer[i];
      convertedBuffer[i * 4 + 1] = buffer[i];
      convertedBuffer[i * 4 + 2] = buffer[i];
      convertedBuffer[i * 4 + 3] = 255;
    }
    break;

  case 3:
    //LOG("Converting image from 3 to 4 channels.\n");
    // Add an opaque channel.
    convertedBuffer = (uint8_t *)AlignedAlloc(w * h * 4 * sizeof(uint8_t));
    for (unsigned i = 0; i < w * h; ++i) {
      convertedBuffer[i * 4 + 0] = buffer[i * 3 + 0];
      convertedBuffer[i * 4 + 1] = buffer[i * 3 + 1];
      convertedBuffer[i * 4 + 2] = buffer[i * 3 + 2];
      convertedBuffer[i * 4 + 3] = 255;
    }
    break;

  case 4:
    break;  // This is the wanted format.

  case 2:
  default:
    LOG("Image had unsuitable number of color components: %d %s\n", c, fileName);
    return false;
  }

  if (convertedBuffer) {
    AlignedFree(buffer);
    buffer = convertedBuffer;
  }

  width = (unsigned)w;
  height = (unsigned)h;

  // Create a bigger canvas if needed to satisfy the pow2 dimension requirement.
  if (convertPow2) {
    unsigned newWidth = RoundUpToPow2(width);
    unsigned newHeight = RoundUpToPow2(height);
    if ((newWidth != width) || (newHeight != height)) {
      LOG("Converting loaded image from (%d, %d) to (%d, %d)\n", width, height, newWidth, newHeight);

      unsigned biggerSize = newWidth * newHeight * 4 * sizeof(uint8_t);
      uint8_t *biggerBuffer = (uint8_t *)AlignedAlloc(biggerSize);

      // Fill it with black.
      memset(biggerBuffer, 0, biggerSize);

      // Copy over the old bitmap.
      // Centered in the new bitmap.
      int offsetX = (newWidth - width) / 2,
          offsetY = (newHeight - height) / 2;
      for (unsigned y = 0; y < height; ++y)
        memcpy(biggerBuffer + (offsetX + (y + offsetY) * newWidth) * 4, buffer + y * width * 4, width * 4);

      // Store the texture coordinate scaling.
      u = width / (float)newWidth;
      v = height / (float)newHeight;

      // Swap the buffers and dimensions.
      AlignedFree(buffer);
      buffer  = biggerBuffer;
      width   = newWidth;
      height  = newHeight;
    }
  }

#if 0 // Fill the alpha channel with transparent gradient alpha for testing
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

  delete [] fileBuffer;
  return !!buffer;
}

unsigned Image::GetSize() const {
  switch (format) {
  case kRGBA32:   return width * height * 4;
  default:        return 0;
  }
}

void Image::Clear(const float *rgba) {
  // Quantize the color to target resolution.
  uint8_t r = (uint8_t)(rgba[0] * 255.0f),
          g = (uint8_t)(rgba[1] * 255.0f),
          b = (uint8_t)(rgba[2] * 255.0f),
          a = (uint8_t)(rgba[3] * 255.0f);

  // Fill out the first line manually.
  for (unsigned w = 0; w < width; ++w) {
    buffer[w * 4 + 0] = r;
    buffer[w * 4 + 1] = g;
    buffer[w * 4 + 2] = b;
    buffer[w * 4 + 3] = a;
  }

  // Copy the first line to the rest of them.
  for (unsigned h = 1; h < height; ++h)
    memcpy(buffer + h * width * 4, buffer, width * 4);
}

void Image::Gradient() {
  // Fill out the first line manually.
  for (unsigned x = 0; x < width; ++x) {
    uint8_t intensity = x > 255 ? 255 : x;
    buffer[x * 4 + 0] = intensity;
    buffer[x * 4 + 1] = intensity;
    buffer[x * 4 + 2] = intensity;
    buffer[x * 4 + 3] = 255;
  }

  // Copy the first line to the rest of them.
  for (unsigned h = 1; h < height; ++h)
    memcpy(buffer + h * width * 4, buffer, width * 4);
}
