#ifndef SHAPE_H
#define SHAPE_H

#include "../base/vecmath.h"
#include "drawable.h"

namespace eng {

class Animatable : public Drawable {
 public:
  Animatable() = default;
  ~Animatable() override = default;

  void Translate(const base::Vector2& pos);
  void Scale(const base::Vector2& scale);
  void Scale(float scale);
  void Rotate(float angle);

  void SetPosition(const base::Vector2& pos) { position_ = pos; }
  void SetSize(const base::Vector2& size) { size_ = size; }
  void SetTheta(float theta);

  base::Vector2 GetPosition() const { return position_; }
  base::Vector2 GetSize() const { return size_ * scale_; }
  float GetTheta() const { return theta_; }
  base::Vector2 GetRotation() const { return rotation_; }

  // Pure virtuals for frame animation support.
  virtual void SetFrame(size_t frame) = 0;
  virtual size_t GetFrame() const = 0;
  virtual size_t GetNumFrames() const = 0;

  virtual void SetColor(const base::Vector4& color) = 0;
  virtual base::Vector4 GetColor() const = 0;

  void PlaceToLeftOf(const Animatable& s) {
    Translate({s.GetSize().x / -2.0f + GetSize().x / -2.0f, 0});
  }

  void PlaceToRightOf(const Animatable& s) {
    Translate({s.GetSize().x / 2.0f + GetSize().x / 2.0f, 0});
  }

  void PlaceToTopOf(const Animatable& s) {
    Translate({0, s.GetSize().y / 2.0f + GetSize().y / 2.0f});
  }

  void PlaceToBottomOf(const Animatable& s) {
    Translate({0, s.GetSize().y / -2.0f + GetSize().y / -2.0f});
  }

 protected:
  base::Vector2 position_ = {0, 0};
  base::Vector2 size_ = {1, 1};
  base::Vector2 scale_ = {1, 1};
  base::Vector2 rotation_ = {0, 1};
  float theta_ = 0;
};

}  // namespace eng

#endif  // SHAPE_H
