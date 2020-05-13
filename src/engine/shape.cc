#include "shape.h"
#include <cmath>

namespace eng {

void Shape::Translate(const Vector2& offset) {
  offset_ += offset;
}

void Shape::Scale(const Vector2& scale) {
  scale_ *= scale;
}

void Shape::Scale(float scale) {
  scale_ *= scale;
}

void Shape::Rotate(float angle) {
  theta_ += angle;
  rotation_.x = sin(theta_);
  rotation_.y = cos(theta_);
}

void Shape::SetTheta(float theta) {
  theta_ = theta;
  rotation_.x = sin(theta_);
  rotation_.y = cos(theta_);
}

}  // namespace eng
