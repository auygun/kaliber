#ifndef FONT_H
#define FONT_H

#include <stdint.h>
#include "../third_party/stb/stb_truetype.h"

class Fontx {
public:
  Fontx();
  ~Fontx();

  bool Create();
  void Destroy();

  void CalculateBoundingBox(const char *text, int &width, int &height);
  void CalculateBoundingBox(const char *text, int &x0, int &y0, int &x1, int &y1);

  void Print(int x, int y, const char *text, uint8_t *buffer, unsigned width);

private:
  enum Constants {
    kGlyphSize  = 512,
    kFirstChar  = 32,   // ' ' (space)
    kNumChars   = 96    // Covers almost all ASCII chars.
  };

  uint8_t         *glyph_cache_;          // Image data.
  stbtt_bakedchar glyph_info_[kNumChars]; // Coordinates and advance.
};

#endif // FONT_H
