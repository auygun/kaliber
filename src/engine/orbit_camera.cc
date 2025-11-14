#include "engine/orbit_camera.h"

#include <algorithm>
#include <cmath>

#include "base/interpolation.h"
#include "base/vecmath.h"
#include "engine/components.h"
#include "engine/ecs.h"
#include "engine/world.h"

using namespace base;

namespace eng {

void OrbitCamera::Init(World& world) {
  player_input_ = &world.GetRegistry().GetSingletonComponent<PlayerInput>();
}

void OrbitCamera::Update(World& world, float delta_time) {
  for (auto [entity, _, orbit_camera, local_transform] :
       world.GetRegistry()
           .View<PrimaryCameraTag, OrbitCameraComponent,
                 LocalTransformComponent>()) {
    float polar = player_input_->mouse_y_delta * orbit_camera.sensitivity;
    float azimuthal = player_input_->mouse_x_delta * orbit_camera.sensitivity;
    float radius = Acceleration(
        player_input_->mouse_scroll_delta * orbit_camera.sensitivity,
        orbit_camera.speed);

    // Rotate
    if ((player_input_->mouse_left_held &&
         (polar != 0.0f || azimuthal != 0.0f)) ||
        radius != 0.0f) {
      orbit_camera.polar =
          std::clamp(orbit_camera.polar + polar, -0.25f, 0.25f);
      orbit_camera.azimuthal =
          std::fmod(orbit_camera.azimuthal + azimuthal, 1.0);
      orbit_camera.radius =
          std::clamp(orbit_camera.radius + radius, 0.0f, 500.0f);

      // Update local transformation
      local_transform.transform.CreateXRotation(orbit_camera.polar);
      local_transform.transform.M_x_RotY(orbit_camera.azimuthal);
    }

    local_transform.transform.Row(3) =
        orbit_camera.center +
        (local_transform.transform.Row(2) * -orbit_camera.radius);

    world.GetRegistry().AddComponent(entity, WorldTransformDirtyTag{});

    break;  // There can be only one primary camera.
  }
}

}  // namespace eng
