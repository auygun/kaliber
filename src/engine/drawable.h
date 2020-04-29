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

  void Scale(const Vector2& scale) {
    scale_ *= scale;
  }

  void Scale(float scale) {
    scale_ *= scale;
  }

  void ResetCenter(const Vector2& offset) {
    center_ = offset_ + offset;
    offset_ += offset;
  }

  void Rotate(float angle) {
    rotation_.x = sin(angle);
    rotation_.y = cos(angle);
  }

  void SetOffset(const Vector2& offset) { offset_ = offset; }
  void SetScale(const Vector2& scale) { scale_ = scale; }
  void SetCenter(const Vector2& center) { center_ = center; }
  void SetRotation(const Vector2& rotation) { rotation_ = rotation; }

  void PlaceToLeftOf(const Drawable& d) {
    Translate({d.scale().x / -2.0f + scale().x / -2.0f, 0});
  }

  void PlaceToRightOf(const Drawable& d) {
    Translate({d.scale().x / 2.0f + scale().x / 2.0f, 0});
  }

  Vector2 offset() const { return offset_; }
  Vector2 scale() const { return scale_; }
  Vector2 center() const { return center_; }
  Vector2 rotation() const { return rotation_; }

 private:
  Vector2 offset_ = {0, 0};
  Vector2 scale_ = {1, 1};
  Vector2 center_ = {0, 0};
  Vector2 rotation_ = {0, 1};
};

}  // namespace engine

#endif  // DRAWABLE_H
