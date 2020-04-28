#ifndef DRAWABLE_H
#define DRAWABLE_H

#include "../base/vecmath.h"
#include <math.h>

namespace engine {

class Drawable {
 public:
  Drawable() = default;
  virtual ~Drawable() = default;

  virtual void Draw() = 0;

  void Translate(const Vector2& offset) {
    offset_ += offset;
  }

  void Scale(float scale) {
    scale_ *= scale;
  }

  void Rotate(float angle) {
    rotation_.x = sin(angle);
    rotation_.y = cos(angle);
  }

  void SetOffset(const Vector2& offset) { offset_ = offset; }
  void SetScale(const Vector2& scale) { scale_ = scale; }
  void SetRotation(const Vector2& rotation) { rotation_ = rotation; }

  Vector2 offset() { return offset_; }
  Vector2 scale() { return scale_; }
  Vector2 rotation() { return rotation_; }

 private:
  Vector2 offset_ = {0, 0};
  Vector2 scale_ = {1, 1};
  Vector2 rotation_ = {0, 1};
};

}  // namespace engine

#endif  // DRAWABLE_H
