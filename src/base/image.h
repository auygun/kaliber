#ifndef IMAGE_H
#define IMAGE_H

#include <stdint.h>

class Image {
public:
  enum Format {
    kRGBA32
  };

  Image();
  ~Image();

  bool Create(unsigned width, unsigned height);
  void Destroy();
  void Copy(const Image &image);

  bool Load(const char *fileName, bool convertPow2 = true);

  unsigned GetWidth() const                       { return width; }
  unsigned GetHeight() const                      { return height; }
  Format GetFormat() const                        { return format; }
  bool IsCompressed() const                       { return format > kRGBA32; }

  unsigned GetSize() const;

  const uint8_t *GetBuffer() const                { return buffer; }
  uint8_t *GetBuffer()                            { return buffer; }

  void Clear(const float *rgba);
  void Gradient();

  void GetUV(float &_u, float &_v) const          { _u = u; _v = v; }


private:
  uint8_t   *buffer;
  unsigned  width,
            height;
  Format    format;
  float     u, v;
};

#endif // IMAGE_H
