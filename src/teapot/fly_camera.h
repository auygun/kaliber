#ifndef TEAPOT_FLY_CAMERA_H
#define TEAPOT_FLY_CAMERA_H

#include <memory>

#include "base/vecmath.h"

namespace eng {

class Scene;
struct PlayerInput;

// Fly Camera System.
class FlyCamera {
 public:
  FlyCamera() = default;
  ~FlyCamera() = default;

  void Init(Scene* scene);

  void Update(Scene* scene);

 private:
  PlayerInput* player_input_{nullptr};

  float last_mouse_x_ = 0.0f;
  float last_mouse_y_ = 0.0f;
};

}  // namespace eng

#endif  // TEAPOT_FLY_CAMERA_H
