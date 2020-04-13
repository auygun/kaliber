#ifndef TEXT_H
#define TEXT_H

#include <stdint.h>
#include "../third_party/stb/stb_truetype.h"

class Image;

namespace engine {

class Font {
public:
  Font();
  ~Font();

  bool Create();
  void Destroy();

  void CalculateBoundingBox(const char *text, int &width, int &height);
  void CalculateBoundingBox(const char *text, int &x0, int &y0, int &x1, int &y1);

  void Print(int x, int y, const char *text, Image &image);

private:
  enum Constants {
    kGlyphSize  = 512,
    kFirstChar  = 32,   // ' ' (space)
    kNumChars   = 96    // Covers almost all ASCII chars.
  };

  uint8_t         *glyphCache;          // Image data.
  stbtt_bakedchar glyphInfo[kNumChars]; // Coordinates and advance.
};

} // namespace engine

#endif // TEXT_H
