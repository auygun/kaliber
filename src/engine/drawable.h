#ifndef DRAWABLE_H
#define DRAWABLE_H

#include "../base/vecmath.h"

namespace engine {

class Drawable {
 public:
  Drawable() = default;
  virtual ~Drawable() = default;

  virtual void Draw() = 0;

  void Translate(const Vector2& offset);
  void Scale(const Vector2& scale);
  void Scale(float scale);
  void Rotate(float angle);

  void SetOffset(const Vector2& offset) { offset_ = offset; }
  void SetScale(const Vector2& scale) { scale_ = scale; }
  void SetPivot(const Vector2& pivot) { pivot_ = pivot; }
  void SetRotation(const Vector2& rotation) { rotation_ = rotation; }
  void SetColor(const Vector4& color) { color_ = color; }
  void SetVisible(bool visible) { visible_ = visible; }

  Vector2 offset() const { return offset_; }
  Vector2 scale() const { return scale_; }
  Vector2 pivot() const { return pivot_; }
  Vector2 rotation() const { return rotation_; }
  Vector4 color() const { return color_; }
  bool visible() const { return visible_; }

 private:
  Vector2 offset_ = {0, 0};
  Vector2 scale_ = {1, 1};
  Vector2 pivot_ = {0, 0};
  Vector2 rotation_ = {0, 1};
  Vector4 color_ = {1, 1, 1, 1};
  bool visible_ = false;
};

}  // namespace engine

#endif  // DRAWABLE_H
