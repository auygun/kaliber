#ifndef DRAWABLE_H
#define DRAWABLE_H

#include "../base/vecmath.h"
#include <cstdlib>

namespace engine {

class Drawable {
 public:
  Drawable() = default;
  virtual ~Drawable() = default;

  virtual void Draw() = 0;

  void Translate(const Vector2& offset) {
    offset_.x += offset.x;
    offset_.y += offset.y;
  }
  void SetOffset(const Vector2& offset) { offset_ = offset; }
  void SetScale(const Vector2& scale) { scale_ = scale; }

  Vector2 offset() { return offset_; }
  Vector2 scale() { return scale_; }

 private:
  Vector2 offset_ = {0, 0};
  Vector2 scale_ = {1, 1};
};

}  // namespace engine

#endif  // DRAWABLE_H
