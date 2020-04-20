#ifndef TEXTURE_H
#define TEXTURE_H

#include "opengl.h"

class Image;

namespace engine {

class Texture {
 public:
  Texture() = default;
  ~Texture();

  bool Create(const Image& image);
  void Destroy();

  void Activate();
  bool Update(const Image& image);

 private:
  GLuint id_ = 0;
};

}  // namespace engine

#endif  // TEXTURE_H
