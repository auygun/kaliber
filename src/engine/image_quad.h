#ifndef IMAGE_QUAD_H
#define IMAGE_QUAD_H

#include "../base/vecmath.h"
#include "renderer/texture.h"
#include "drawable.h"

#include <string>
#include <vector>

namespace engine {

class Image;

class ImageQuad : public Drawable {
 public:
  ImageQuad() = default;
  ~ImageQuad() override = default;

  bool Create(std::shared_ptr<const Image> image);

  void Draw() override;

 private:
  Texture texture_;
  Vector2 uv_scale_ = {1, 1};
};

}  // namespace engine

#endif  // IMAGE_QUAD_H
