#ifndef FONT_H
#define FONT_H

#include <stdint.h>
#include <memory>
#include <string>

#include "../third_party/stb/stb_truetype.h"
#include "asset.h"

namespace eng {

class Font : public Asset {
 public:
  Font() = default;
  ~Font() override = default;

  bool Load(const std::string& file_name) override;

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
    kNumChars = 96    // Covers almost all ASCII chars.
  };

  std::unique_ptr<uint8_t[]> glyph_cache_;  // Image data.
  stbtt_bakedchar glyph_info_[kNumChars];   // Coordinates and advance.

  int line_height_ = 0;
  int yoff_ = 0;
};

}  // namespace eng

#endif  // FONT_H
