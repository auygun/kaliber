#ifndef ENGINE_ASSET_FONT_H
#define ENGINE_ASSET_FONT_H

#include <cstdint>
#include <memory>
#include <string>

#include "third_party/stb/stb_truetype.h"

namespace eng {

class Font {
 public:
  Font() = default;
  ~Font() = default;

  bool Load(const std::string& file_name);

  void CalculateBoundingBox(const std::string& text,
                            int& width,
                            int& height) const;
  void CalculateBoundingBox(const std::string& text,
                            int& x0,
                            int& y0,
                            int& x1,
                            int& y1) const;

  void Print(int x,
             int y,
             const std::string& text,
             uint8_t* buffer,
             int width) const;

  int GetLineHeight() const { return line_height_; }

  bool IsValid() const { return !!glyph_cache_; }

 private:
  enum Constants {
    kGlyphSize = 512,
    kFirstChar = 32,  // ' ' (space)
    kNumChars = 224   // Covers all ASCII chars.
  };

  std::unique_ptr<uint8_t[]> glyph_cache_;  // Image data.
  stbtt_bakedchar glyph_info_[kNumChars];   // Coordinates and advance.

  int line_height_ = 0;
  int yoff_ = 0;
};

}  // namespace eng

#endif  // ENGINE_ASSET_FONT_H
