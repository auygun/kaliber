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
};

}  // namespace eng

#endif  // TEAPOT_FLY_CAMERA_H
