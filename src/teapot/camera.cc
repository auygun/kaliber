#include "teapot/camera.h"

#include <algorithm>

using namespace base;

Camera::Camera(const Camera& other)
    : center_{0},
      radius_(other.radius_),
      polar_(other.polar_),
      azimuthal_(other.azimuthal_),
      matrix_(other.matrix_) {}

void Camera::Create(const Vector3f& center,
                    float polar,
                    float azimuthal,
                    float radius) {
  center_ = center;
  polar_ = std::clamp(polar, -0.25f, 0.25f);
  azimuthal_ = fmod(azimuthal, 1.0);
  radius_ = std::clamp(radius, 0.5f, 300.0f);
  MakeMatrix();
}

void Camera::Move(const Vector3f& delta) {
  if (debug_cam_) {
    debug_cam_->Move(delta);
    return;
  }

  center_ += delta;
  matrix_.Row(3) = center_ + (matrix_.Row(2) * -radius_);
}

void Camera::Orbit(float polar, float azimuthal, float radius) {
  if (debug_cam_) {
    debug_cam_->Orbit(polar, azimuthal, radius);
    return;
  }

  if (polar == 0 && azimuthal == 0 && radius == 0)
    return;

  polar_ = std::clamp(polar_ + polar, -0.25f, 0.25f);
  azimuthal_ = fmod(azimuthal_ + azimuthal, 1.0);
  radius_ = std::clamp(radius_ + radius, 0.5f, 300.0f);
  MakeMatrix();
}

const base::Matrix4f& Camera::GetMatrix() const {
  return debug_cam_ ? debug_cam_->matrix_ : matrix_;
}

void Camera::ToggleDebugCamera() {
  if (debug_cam_) {
    debug_cam_.reset();
  } else {
    debug_cam_ = std::make_unique<Camera>(*this);
  }
}

void Camera::MakeMatrix() {
  matrix_.CreateXRotation(polar_);
  matrix_.M_x_RotY(azimuthal_);
  matrix_.Row(3) = center_ + (matrix_.Row(2) * -radius_);
}
