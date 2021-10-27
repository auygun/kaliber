#include "engine/animatable.h"

#include <cmath>

using namespace base;

namespace eng {

void Animatable::Translate(const Vector2f& pos) {
  position_ += pos;
}

void Animatable::Scale(const Vector2f& scale) {
  scale_ = scale;
}

void Animatable::Scale(float scale) {
  scale_ = {scale, scale};
}

void Animatable::Rotate(float angle) {
  theta_ += angle;
  rotation_.x = sin(theta_);
  rotation_.y = cos(theta_);
}

void Animatable::SetTheta(float theta) {
  theta_ = theta;
  rotation_.x = sin(theta_);
  rotation_.y = cos(theta_);
}

}  // namespace eng
