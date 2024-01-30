#include "teapot/camera.h"

#include <algorithm>

using namespace base;

void Camera::Create(const Vector3f& center,
                    float polar,
                    float azimuthal,
                    float radius) {
  center_ = center;
  polar_ = std::clamp(polar, -0.25f, 0.25f);
  azimuthal_ = fmod(azimuthal, 1.0);
  radius_ = std::clamp(radius, 5.0f, 50.0f);
  MakeMatrix();
}

void Camera::Move(const Vector3f& delta) {
  center_ += delta;
  matrix_.Row(3) = center_ + (matrix_.Row(2) * -radius_);
}

void Camera::Orbit(float polar, float azimuthal, float radius) {
  if (polar == 0 && azimuthal == 0 && radius == 0)
    return;

  polar_ = std::clamp(polar_ + polar, -0.25f, 0.25f);
  azimuthal_ = fmod(azimuthal_ + azimuthal, 1.0);
  radius_ = std::clamp(radius_ + radius, 5.0f, 100.0f);
  MakeMatrix();
}

void Camera::MakeMatrix() {
  matrix_.CreateXRotation(polar_);
  matrix_.M_x_RotY(azimuthal_);
  matrix_.Row(3) = center_ + (matrix_.Row(2) * -radius_);
}
