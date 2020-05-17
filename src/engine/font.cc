#include "font.h"
#include "engine.h"
#include <stdint.h>
#include "../base/file.h"
#include "../base/log.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "../third_party/stb/stb_truetype.h"

namespace eng {

Font::Font() : glyph_cache_(nullptr) {}

Font::~Font() {
  immutable_ = false;
  Destroy();
}

bool Font::Create(const std::string& font_name) {
  Destroy();

  std::string full_path = "fonts/";
  full_path += font_name;

  // Read the font file.
  int buffer_size = 0;
  char* buffer = File::ReadWholeFile(full_path.c_str(),
      Engine::Get().GetRootPath().c_str(), &buffer_size);
  if (!buffer) {
    LOG << "Failed to read font file.";
    return false;
  }

  bool result = false;
  do {
    // Allocate a cache bitmap for the glyphs.
    // This is one 8 bit channel intensity data.
    // It's tighly packed.
    glyph_cache_ = new uint8_t[kGlyphSize * kGlyphSize];
    if (!glyph_cache_) {
      LOG << "Failed to allocate glyph cache.";
      break;
    }

    // Rasterize glyphs and pack them into the cache.
    const float kFontHeight = 32.0f;
    if (stbtt_BakeFontBitmap((unsigned char*)buffer, 0, kFontHeight,
                             glyph_cache_, kGlyphSize, kGlyphSize, kFirstChar,
                             kNumChars, glyph_info_) <= 0) {
      LOG << "Failed to bake the glyph cache: " << result;
      break;
    }

    result = true;
  } while (0);

  delete[] buffer;

  int x0, y0, x1, y1;
  CalculateBoundingBox("Ilfgjy", x0, y0, x1, y1);
  line_height_ = y1 - y0;
  vertical_shift_ = -y0;

  return result;
}

void Font::Destroy() {
  if (glyph_cache_) {
    delete[] glyph_cache_;
    glyph_cache_ = NULL;
  }
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
  // LOG << "-- StretchBlit: --";
  // LOG << "dst: rect(" << dst_x0 << ", " << dst_y0 << ")..("
  //     << dst_x1 << ".." << dst_y1 << "), pitch(" << dst_pitch << ")";
  // LOG << "src: rect(" << src_x0 << ", " << src_y0 << ")..("
  //     << src_x1 << ".." << src_y1 << "), pitch(" << src_pitch << ")";

  int dst_width = dst_x1 - dst_x0, dst_height = dst_y1 - dst_y0,
      src_width = src_x1 - src_x0, src_height = src_y1 - src_y0;

  // int dst_dx = dst_width > 0 ? 1 : -1,
  //     dst_dy = dst_height > 0 ? 1 : -1;

  // LOG << "dst_width = " << dst_width << ", dst_height = " << dst_height;
  // LOG << "src_width = " << src_width << ", src_height = " << src_height;

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

void Font::CalculateBoundingBox(const char* text,
                                 int& x0,
                                 int& y0,
                                 int& x1,
                                 int& y1) const {
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

void Font::CalculateBoundingBox(const char* text,
                                int& width,
                                int& height) const {
  int x0, y0, x1, y1;
  CalculateBoundingBox(text, x0, y0, x1, y1);
  width = x1 - x0;
  height = y1 - y0;
  // LOG << "width = " << width << ", height = " << height;
}

void Font::Print(int x,
                  int y,
                  const char* text,
                  uint8_t* buffer,
                  int width) {
  // LOG("Font::Print() = %s\n", text);

  float fx = (float)x, fy = (float)y + (float)vertical_shift_;

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

}  // namespace eng
