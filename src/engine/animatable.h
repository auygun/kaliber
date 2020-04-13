#ifndef SHAPE_H
#define SHAPE_H

#include "../base/vecmath.h"

namespace eng {

class Animatable {
 public:
  Animatable() = default;
  virtual ~Animatable() = default;

  void Translate(const base::Vector2& offset);
  void Scale(const base::Vector2& scale);
  void Scale(float scale);
  void Rotate(float angle);

  void SetOffset(const base::Vector2& offset) { offset_ = offset; }
  void SetScale(const base::Vector2& scale) { scale_ = scale; }
  void SetPivot(const base::Vector2& pivot) { pivot_ = pivot; }
  void SetTheta(float theta);

  base::Vector2 GetOffset() const { return offset_; }
  base::Vector2 GetScale() const { return scale_; }
  base::Vector2 GetPivot() const { return pivot_; }
  float GetTheta() const { return theta_; }

  // Pure virtuals for frame animation support.
  virtual void SetFrame(size_t frame) = 0;
  virtual size_t GetFrame() = 0;
  virtual size_t GetNumFrames() = 0;

  virtual void SetColor(const base::Vector4& color) = 0;
  virtual base::Vector4 GetColor() const = 0;

  void SetVisible(bool visible) { visible_ = visible; }
  bool IsVisible() const { return visible_; }

  void PlaceToLeftOf(const Animatable& s) {
    Translate({s.GetScale().x / -2.0f + GetScale().x / -2.0f, 0});
  }

  void PlaceToRightOf(const Animatable& s) {
    Translate({s.GetScale().x / 2.0f + GetScale().x / 2.0f, 0});
  }

  void PlaceToTopOf(const Animatable& s) {
    Translate({0, s.GetScale().y / 2.0f + GetScale().y / 2.0f});
  }

  void PlaceToBottomOf(const Animatable& s) {
    Translate({0, s.GetScale().y / -2.0f + GetScale().y / -2.0f});
  }

 protected:
  base::Vector2 offset_ = {0, 0};
  base::Vector2 scale_ = {1, 1};
  base::Vector2 pivot_ = {0, 0};
  base::Vector2 rotation_ = {0, 1};
  float theta_ = 0;
  bool visible_ = false;
};

}  // namespace eng

#endif  // SHAPE_H
