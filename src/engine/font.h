#ifndef FONT_H
#define FONT_H

#include <stdint.h>
#include "asset.h"
#include "../third_party/stb/stb_truetype.h"
#include <string>
#include <memory>

namespace eng {

class Font : public Asset {
 public:
  Font();
  ~Font() override;

  bool Create(const std::string& file_name);
  void Destroy();

  void CalculateBoundingBox(const char* text, int& width, int& height) const;
  void CalculateBoundingBox(const char* text,
                            int& x0,
                            int& y0,
                            int& x1,
                            int& y1) const;

  void Print(int x, int y, const char* text, uint8_t* buffer, int width);

  int GetLineHeight() const { return line_height_; }

  bool IsValid() const { return !!glyph_cache_; }

 private:
  enum Constants {
    kGlyphSize = 512,
    kFirstChar = 32,  // ' ' (space)
    kNumChars = 96    // Covers almost all ASCII chars.
  };

  std::unique_ptr<uint8_t[]> glyph_cache_; // Image data.
  stbtt_bakedchar glyph_info_[kNumChars];  // Coordinates and advance.

  int line_height_ = 0;
  int vertical_shift_ = 0;
};

}  // namespace eng

#endif  // FONT_H
