#ifndef TEAPOT_CAMERA_H
#define TEAPOT_CAMERA_H

#include "base/vecmath.h"

class Camera {
 public:
  Camera() = default;
  ~Camera() = default;

  void Create(const base::Vector3f& center,
              float polar,
              float azimuthal,
              float radius);

  void Move(const base::Vector3f& delta);
  void Orbit(float polar, float azimuthal, float radius);

  const base::Matrix4f& GetMatrix() const { return matrix_; }

 private:
  base::Vector3f center_{0};
  float radius_ = 0;
  float polar_ = 0;
  float azimuthal_ = 0;
  base::Matrix4f matrix_{1};

  void MakeMatrix();
};

#endif  // TEAPOT_CAMERA_H
