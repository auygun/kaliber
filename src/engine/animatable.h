#ifndef ENGINE_SHAPE_H
#define ENGINE_SHAPE_H

#include "base/vecmath.h"
#include "engine/drawable.h"

namespace eng {

class Animatable : public Drawable {
 public:
  Animatable() = default;
  ~Animatable() override = default;

  void Translate(const base::Vector2f& pos);
  void Scale(const base::Vector2f& scale);
  void Scale(float scale);
  void Rotate(float angle);

  void SetPosition(const base::Vector2f& pos) { position_ = pos; }
  void SetSize(const base::Vector2f& size) { size_ = size; }
  void SetTheta(float theta);

  base::Vector2f GetPosition() const { return position_; }
  base::Vector2f GetSize() const { return size_ * scale_; }
  float GetTheta() const { return theta_; }
  base::Vector2f GetRotation() const { return rotation_; }

  // Pure virtuals for frame animation support.
  virtual void SetFrame(size_t frame) = 0;
  virtual size_t GetFrame() const = 0;
  virtual size_t GetNumFrames() const = 0;

  virtual void SetColor(const base::Vector4f& color) = 0;
  virtual base::Vector4f GetColor() const = 0;

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
  base::Vector2f position_ = {0, 0};
  base::Vector2f size_ = {0, 0};
  base::Vector2f scale_ = {1, 1};
  base::Vector2f rotation_ = {0, 1};
  float theta_ = 0;
};

}  // namespace eng

#endif  // ENGINE_SHAPE_H
