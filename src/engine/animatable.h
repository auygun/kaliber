#ifndef SHAPE_H
#define SHAPE_H

#include "../base/vecmath.h"

namespace eng {

class Animatable {
 public:
  Animatable() = default;
  virtual ~Animatable() = default;

  Animatable(const Animatable&) = delete;
  Animatable& operator=(const Animatable&) = delete;

  void Translate(const Vector2& offset);
  void Scale(const Vector2& scale);
  void Scale(float scale);
  void Rotate(float angle);

  void SetOffset(const Vector2& offset) { offset_ = offset; }
  void SetScale(const Vector2& scale) { scale_ = scale; }
  void SetPivot(const Vector2& pivot) { pivot_ = pivot; }
  void SetTheta(float theta);
  void SetColor(const Vector4& color) { color_ = color; }

  Vector2 GetOffset() const { return offset_; }
  Vector2 GetScale() const { return scale_; }
  Vector2 GetPivot() const { return pivot_; }
  float GetTheta() const { return theta_; }
  Vector4 GetColor() const { return color_; }

  // Pure virtuals for frame animation support.
  virtual void SetFrame(size_t frame) = 0;
  virtual size_t GetFrame() = 0;
  virtual size_t GetNumFrames() = 0;

  void PlaceToLeftOf(const Animatable& s) {
    Translate({s.GetScale().x / -2.0f + GetScale().x / -2.0f, 0});
  }

  void PlaceToRightOf(const Animatable& s) {
    Translate({s.GetScale().x / 2.0f + GetScale().x / 2.0f, 0});
  }

  void PlaceToTopOf(const Animatable& s) {
    Translate({0 ,s.GetScale().y / 2.0f + GetScale().y / 2.0f});
  }

  void PlaceToBottomOf(const Animatable& s) {
    Translate({0 ,s.GetScale().y / -2.0f + GetScale().y / -2.0f});
  }

 protected:
  Vector2 offset_ = {0, 0};
  Vector2 scale_ = {1, 1};
  Vector2 pivot_ = {0, 0};
  Vector2 rotation_ = {0, 1};
  float theta_ = 0;
  Vector4 color_ = {1, 1, 1, 1};
};

}  // namespace eng

#endif  // SHAPE_H
