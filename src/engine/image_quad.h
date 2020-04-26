#ifndef IMAGE_QUAD_H
#define IMAGE_QUAD_H

#include "../base/vecmath.h"
#include "renderer/texture.h"
#include "drawable.h"

#include <string>
#include <vector>

class Image;

namespace engine {

class ImageQuad : public Drawable {
 public:
  ImageQuad() = default;
  ~ImageQuad() override = default;

  bool Create(std::unique_ptr<Image> image);

  void Draw() override;

  void Translate(const Vector2& offset) { offset_ = offset; }
  void Scale(const Vector2& scale) { scale_ = scale; }

  Vector2 offset() { return offset_; }
  Vector2 scale() { return scale_; }

 private:
  Texture texture_;
  Vector2 offset_ = {0, 0};
  Vector2 scale_ = {1, 1};
};

}  // namespace engine

#endif  // IMAGE_QUAD_H
