#include "font.h"
#include <stdint.h>
#include "file.h"
#include "log.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "../third_party/stb/stb_truetype.h"

Fontx::Fontx() : glyph_cache_(NULL) {}

Fontx::~Fontx() {
  Destroy();
}

bool Fontx::Create() {
  Destroy();

  // Read the font file.
  unsigned bufferSize = 0;
  char* buffer = File::ReadWholeFile("fonts/Roboto-Regular.ttf", &bufferSize);
  if (!buffer) {
    LOG("Failed to read font file.\n");
    return false;
  }

  bool result = false;
  do {
    // Allocate a cache bitmap for the glyphs.
    // This is one 8 bit channel intensity data.
    // It's tighly packed.
    glyph_cache_ = new uint8_t[kGlyphSize * kGlyphSize];
    if (!glyph_cache_) {
      LOG("Failed to allocate glyph cache.\n");
      break;
    }

    // Rasterize glyphs and pack them into the cache.
    const float kFontHeight = 32.0f;
    if (stbtt_BakeFontBitmap((unsigned char*)buffer, 0, kFontHeight,
                             glyph_cache_, kGlyphSize, kGlyphSize, kFirstChar,
                             kNumChars, glyph_info_) <= 0) {
      LOG("Failed to bake the glyph cache: %d\n", result);
      break;
    }

    result = true;
  } while (0);

  delete[] buffer;
  return result;
}

void Fontx::Destroy() {
  delete[] glyph_cache_;
  glyph_cache_ = NULL;
}

static void StretchBlit_I8_to_RGBA32(int dst_x0,
                                     int dst_y0,
                                     int dst_x1,
                                     int dst_y1,
                                     int src_x0,
                                     int src_y0,
                                     int src_x1,
                                     int src_y1,
                                     uint8_t* dst_rgba,
                                     int dst_pitch,
                                     const uint8_t* src_i,
                                     int src_pitch) {
  // LOG("-- StretchBlit: --\n"
  //     "dst: rect(%d, %d)..(%d..%d), pitch(%d)\n"
  //     "src: rect(%d, %d)..(%d..%d), pitch(%d)\n",
  //     dst_x0, dst_y0, dst_x1, dst_y1, dst_pitch,
  //     src_x0, src_y0, src_x1, src_y1, src_pitch);

  int dst_width = dst_x1 - dst_x0, dst_height = dst_y1 - dst_y0,
      src_width = src_x1 - src_x0, src_height = src_y1 - src_y0;

  // int dst_dx = dst_width > 0 ? 1 : -1,
  //     dst_dy = dst_height > 0 ? 1 : -1;

  // LOG("dst_width  = %d, src_width  = %d\n"
  //     "dst_height = %d, src_height = %d\n",
  //     dst_width, src_width,
  //     dst_height, src_height);

  uint8_t* dst = dst_rgba + (dst_x0 + dst_y0 * dst_pitch) * 4;
  const uint8_t* src = src_i + (src_x0 + src_y0 * src_pitch) * 1;

  // First check if we have to stretch at all.
  if ((dst_width == src_width) && (dst_height == src_height)) {
    // No, straight blit then.
    for (int y = 0; y < dst_height; ++y) {
      for (int x = 0; x < dst_width; ++x) {
        // Alpha test, no blending for now.
        if (src[x]) {
          dst[x * 4 + 0] = src[x];
          dst[x * 4 + 1] = src[x];
          dst[x * 4 + 2] = src[x];
          dst[x * 4 + 3] = 255;
        }
      }

      dst += dst_pitch * 4;
      src += src_pitch * 1;
    }
  } else {
    // ToDo
  }
}

void Fontx::CalculateBoundingBox(const char* text,
                                 int& x0,
                                 int& y0,
                                 int& x1,
                                 int& y1) {
  x0 = 0;
  y0 = 0;
  x1 = 0;
  y1 = 0;

  float x = 0, y = 0;

  while (*text) {
    if (*text >= kFirstChar /*&& *text < (kFirstChar + kNumChars)*/) {
      stbtt_aligned_quad q;
      stbtt_GetBakedQuad(glyph_info_, kGlyphSize, kGlyphSize,
                         *text - kFirstChar, &x, &y, &q, 1);

      int ix0 = (int)q.x0, iy0 = (int)q.y0, ix1 = (int)q.x1, iy1 = (int)q.y1;

      if (ix0 < x0)
        x0 = ix0;
      if (iy0 < y0)
        y0 = iy0;
      if (ix1 > x1)
        x1 = ix1;
      if (iy1 > y1)
        y1 = iy1;

      ++text;
    }
  }
}

void Fontx::CalculateBoundingBox(const char* text, int& width, int& height) {
  int x0, y0, x1, y1;
  CalculateBoundingBox(text, x0, y0, x1, y1);
  width = x1 - x0;
  height = y1 - y0;
}

void Fontx::Print(int x,
                  int y,
                  const char* text,
                  uint8_t* buffer,
                  unsigned width) {
  // LOG("Font::Print() = %s\n", text);

  float fx = (float)x, fy = (float)y;

  while (*text) {
    if (*text >= kFirstChar /*&& *text < (kFirstChar + kNumChars)*/) {
      stbtt_aligned_quad q;
      stbtt_GetBakedQuad(glyph_info_, kGlyphSize, kGlyphSize,
                         *text - kFirstChar, &fx, &fy, &q, 1);

      // LOG("-- glyph --\nxy = (%f %f) .. (%f %f)\nuv = (%f %f) .. (%f %f)\n",
      //     q.x0, q.y0, q.x1, q.y1, q.s0, q.t0, q.s1, q.t1);

      int ix0 = (int)q.x0, iy0 = (int)q.y0, ix1 = (int)q.x1, iy1 = (int)q.y1,
          iu0 = (int)(q.s0 * kGlyphSize), iv0 = (int)(q.t0 * kGlyphSize),
          iu1 = (int)(q.s1 * kGlyphSize), iv1 = (int)(q.t1 * kGlyphSize);

      StretchBlit_I8_to_RGBA32(ix0, iy0, ix1, iy1, iu0, iv0, iu1, iv1, buffer,
                               width, glyph_cache_, kGlyphSize);

      ++text;
    }
  }
}
