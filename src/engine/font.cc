#include "engine/font.h"

#include <codecvt>
#include <locale>

#include "base/log.h"
#include "engine/engine.h"
#include "engine/platform/asset_file.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "../third_party/stb/stb_truetype.h"

namespace eng {

bool Font::Load(const std::string& file_name) {
  // Read the font file.
  size_t buffer_size = 0;
  auto buffer = AssetFile::ReadWholeFile(
      file_name.c_str(), Engine::Get().GetRootPath().c_str(), &buffer_size);
  if (!buffer) {
    LOG << "Failed to read font file.";
    return false;
  }

  do {
    // Allocate a cache bitmap for the glyphs.
    // This is one 8 bit channel intensity data.
    // It's tighly packed.
    glyph_cache_ = std::make_unique<uint8_t[]>(kGlyphSize * kGlyphSize);
    if (!glyph_cache_) {
      LOG << "Failed to allocate glyph cache.";
      break;
    }

    // Rasterize glyphs and pack them into the cache.
    const float kFontHeight = 32.0f;
    if (stbtt_BakeFontBitmap((unsigned char*)buffer.get(), 0, kFontHeight,
                             glyph_cache_.get(), kGlyphSize, kGlyphSize,
                             kFirstChar, kNumChars, glyph_info_) <= 0) {
      LOG << "Failed to bake the glyph cache: ";
      glyph_cache_.reset();
      break;
    }

    int x0, y0, x1, y1;
    CalculateBoundingBox("`IlfKgjy_{)", x0, y0, x1, y1);
    line_height_ = y1 - y0;
    yoff_ = -y0;

    return true;
  } while (false);

  glyph_cache_.reset();

  return false;
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
#if 0
          dst[x * 4 + 0] = src[x];
          dst[x * 4 + 1] = src[x];
          dst[x * 4 + 2] = src[x];
          dst[x * 4 + 3] = 255;
#else
          dst[x * 4 + 3] = src[x];
#endif
        }
      }

      dst += dst_pitch * 4;
      src += src_pitch * 1;
    }
  } else {
    // ToDo
  }
}

void Font::CalculateBoundingBox(const std::string& text,
                                int& x0,
                                int& y0,
                                int& x1,
                                int& y1) const {
  x0 = 0;
  y0 = 0;
  x1 = 0;
  y1 = 0;

  if (!glyph_cache_)
    return;

  float x = 0, y = 0;

  std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> convert;
  std::u16string u16text = convert.from_bytes(text);
  const char16_t* ptr = u16text.c_str();

  while (*ptr) {
    if (*ptr >= kFirstChar && *ptr < (kFirstChar + kNumChars)) {
      stbtt_aligned_quad q;
      stbtt_GetBakedQuad(glyph_info_, kGlyphSize, kGlyphSize, *ptr - kFirstChar,
                         &x, &y, &q, 1);

      int ix0 = (int)q.x0, iy0 = (int)q.y0, ix1 = (int)q.x1, iy1 = (int)q.y1;

      if (ix0 < x0)
        x0 = ix0;
      if (iy0 < y0)
        y0 = iy0;
      if (ix1 > x1)
        x1 = ix1;
      if (iy1 > y1)
        y1 = iy1;
    }
    ++ptr;
  }
}

void Font::CalculateBoundingBox(const std::string& text,
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
                 const std::string& text,
                 uint8_t* buffer,
                 int width) const {
  // LOG("Font::Print() = %s\n", text);

  if (!glyph_cache_)
    return;

  float fx = (float)x, fy = (float)y + (float)yoff_;

  std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> convert;
  std::u16string u16text = convert.from_bytes(text);
  const char16_t* ptr = u16text.c_str();

  while (*ptr) {
    if (*ptr >= kFirstChar && *ptr < (kFirstChar + kNumChars)) {
      stbtt_aligned_quad q;
      stbtt_GetBakedQuad(glyph_info_, kGlyphSize, kGlyphSize, *ptr - kFirstChar,
                         &fx, &fy, &q, 1);

      // LOG("-- glyph --\nxy = (%f %f) .. (%f %f)\nuv = (%f %f) .. (%f %f)\n",
      //     q.x0, q.y0, q.x1, q.y1, q.s0, q.t0, q.s1, q.t1);

      int ix0 = (int)q.x0, iy0 = (int)q.y0, ix1 = (int)q.x1, iy1 = (int)q.y1,
          iu0 = (int)(q.s0 * kGlyphSize), iv0 = (int)(q.t0 * kGlyphSize),
          iu1 = (int)(q.s1 * kGlyphSize), iv1 = (int)(q.t1 * kGlyphSize);

      StretchBlit_I8_to_RGBA32(ix0, iy0, ix1, iy1, iu0, iv0, iu1, iv1, buffer,
                               width, glyph_cache_.get(), kGlyphSize);
    }
    ++ptr;
  }
}

}  // namespace eng
