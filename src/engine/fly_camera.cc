#include "engine/fly_camera.h"

#include <algorithm>
#include <cmath>

#include "base/vecmath.h"
#include "engine/components.h"
#include "engine/ecs.h"
#include "engine/world.h"

using namespace base;

namespace eng {

void FlyCamera::Init(World& world) {
  player_input_ = &world.GetRegistry().GetSingletonComponent<PlayerInput>();
}

void FlyCamera::Update(World& world, float delta_time) {
  for (auto [entity, _, fly_camera, local_transform] :
       world.GetRegistry()
           .View<PrimaryCameraTag, FlyCameraComponent,
                 LocalTransformComponent>()) {
    float pitch = player_input_->mouse_y_delta * fly_camera.sensitivity;
    float yaw = player_input_->mouse_x_delta * fly_camera.sensitivity;

    // Rotate
    if (player_input_->mouse_left_held && (pitch != 0.0f || yaw != 0.0f)) {
      fly_camera.pitch = std::clamp(fly_camera.pitch - pitch, -0.25f, 0.25f);
      fly_camera.yaw = std::fmod(fly_camera.yaw - yaw, 1.0);

      // Update local transformation
      Vector3f pos = local_transform.transform.Row(3);
      local_transform.transform.CreateXRotation(fly_camera.pitch);
      local_transform.transform.M_x_RotY(fly_camera.yaw);
      local_transform.transform.Row(3) = pos;
    }

    Vector3f offset{0};
    float velocity = fly_camera.speed * delta_time;
    if (player_input_->keys_held[static_cast<int>(Key::W)])
      offset += {0, 0, velocity};
    else if (player_input_->keys_held[static_cast<int>(Key::S)])
      offset += {0, 0, -velocity};
    if (player_input_->keys_held[static_cast<int>(Key::Q)])
      offset += {-velocity, 0, 0};
    else if (player_input_->keys_held[static_cast<int>(Key::E)])
      offset += {velocity, 0, 0};

    // Move
    if (offset != 0.0f) {
      local_transform.transform.Row(3) +=
          local_transform.transform.Row(2) * offset.z;
      local_transform.transform.Row(3) +=
          local_transform.transform.Row(0) * offset.x;
    }

    world.GetRegistry().AddComponent(entity, WorldTransformDirtyTag{});

    break;  // There can be only one primary camera.
  }
}

}  // namespace eng
