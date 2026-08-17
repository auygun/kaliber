#ifndef ENGINE_FLY_CAMERA_H
#define ENGINE_FLY_CAMERA_H

#include <memory>

#include "engine/system.h"

namespace eng {

class World;
struct PlayerInput;

// Fly Camera System.
class FlyCamera final : public System {
 public:
  FlyCamera() = default;
  ~FlyCamera() final = default;

  void Init(World& world) final;

  void FixedUpdate(World& world) final {};

  void Update(World& world, float delta_time) final;

 private:
  PlayerInput* player_input_{nullptr};
};

}  // namespace eng

#endif  // ENGINE_FLY_CAMERA_H
