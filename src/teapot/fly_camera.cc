#include "teapot/fly_camera.h"

#include <algorithm>
#include <cmath>

#include "teapot/components.h"
#include "teapot/ecs.h"
#include "teapot/scene.h"

using namespace base;

namespace eng {

void FlyCamera::Init(Scene* scene) {
  player_input_ = &scene->GetRegistry().GetSingletonComponent<PlayerInput>();
}

void FlyCamera::Update(Scene* scene) {
  const Vector3f& offset{0};
  float pitch = player_input_->move_x;
  float yaw = player_input_->move_y;

  for (auto [entity, _, fly_camera, local_transform] :
       scene->GetRegistry()
           .View<PrimaryCameraTag, FlyCameraComponent,
                 LocalTransformComponent>()) {
    // Rotate
    if (pitch != 0 && yaw != 0) {
      fly_camera.pitch = std::clamp(fly_camera.pitch + pitch, -0.25f, 0.25f);
      fly_camera.yaw = std::fmod(fly_camera.yaw + yaw, 1.0);
    }

    // Update local transformation
    Vector3f pos = local_transform.transform.Row(3);
    local_transform.transform.CreateXRotation(fly_camera.pitch);
    local_transform.transform.M_x_RotY(fly_camera.yaw);
    local_transform.transform.Row(3) = pos;

    // Move
    local_transform.transform.Row(3) +=
        local_transform.transform.Row(2) * offset.z;
    local_transform.transform.Row(3) +=
        local_transform.transform.Row(0) * offset.x;

    break;  // There can be only one primary camera.
  }
}

}  // namespace eng
