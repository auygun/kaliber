#include "teapot/camera.h"

#include <algorithm>
#include <cmath>

using namespace base;

Camera::Camera(const Camera& other)
    : polar_(other.polar_),
      azimuthal_(other.azimuthal_),
      matrix_(other.matrix_) {}

void Camera::Create(const Vector3f& position, float polar, float azimuthal) {
  polar_ = std::clamp(polar, -0.25f, 0.25f);
  azimuthal_ = fmod(azimuthal, 1.0);
  MakeMatrix();
  matrix_.Row(3) = position;
}

void Camera::Move(const Vector3f& offset) {
  if (debug_cam_) {
    debug_cam_->Move(offset);
    return;
  }

  // center_ += offset;
  matrix_.Row(3) += matrix_.Row(2) * offset.z;
  matrix_.Row(3) += matrix_.Row(0) * offset.x;
}

void Camera::Rotate(float polar, float azimuthal) {
  if (debug_cam_) {
    debug_cam_->Rotate(polar, azimuthal);
    return;
  }

  if (polar == 0 && azimuthal == 0)
    return;

  polar_ = std::clamp(polar_ + polar, -0.25f, 0.25f);
  azimuthal_ = std::fmod(azimuthal_ + azimuthal, 1.0);
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
  Vector3f pos = matrix_.Row(3);
  matrix_.CreateXRotation(polar_);
  matrix_.M_x_RotY(azimuthal_);
  matrix_.Row(3) = pos;
}
