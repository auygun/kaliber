#ifndef TEAPOT_CAMERA_H
#define TEAPOT_CAMERA_H

#include <memory>

#include "base/vecmath.h"

class Camera {
 public:
  Camera() = default;
  ~Camera() = default;

  Camera(const Camera& other);

  void Create(const base::Vector3f& center, float polar, float azimuthal);

  void Move(const base::Vector3f& offset);
  void Rotate(float polar, float azimuthal);

  // Returns debug camera matrix when it's active.
  const base::Matrix4f& GetMatrix() const;

  // Always returns the main camera matrix (not debug).
  const base::Matrix4f& GetMatrixMainCam() const { return matrix_; }

  void ToggleDebugCamera();

 private:
  float polar_ = 0;
  float azimuthal_ = 0;
  base::Matrix4f matrix_{1};

  std::unique_ptr<Camera> debug_cam_;

  void MakeMatrix();
};

#endif  // TEAPOT_CAMERA_H
