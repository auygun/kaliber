#ifndef PLAYER_H
#define PLAYER_H

#include "../base/vecmath.h"
#include "game_object.h"
#include "../engine/image_quad.h"
#include "../engine/frame_animator.h"

namespace engine {
class InputEvent;
} //  namespace engine

class Player : public GameObject {
 public:
  Player() = default;
  ~Player() override = default;

  bool Initialize() override;

  void Update(float delta_time) override;

  void OnInputEvent(std::unique_ptr<engine::InputEvent> event);

  void Fire();

 private:
  engine::ImageQuad weapon_[2];
  engine::ImageQuad beam_[2];
  engine::FrameAnimator weapon_animator_[2];

  Vector2 drag_start_ = {0, 0};
  Vector2 drag_end_ = {0, 0};

  void CreateWeapon();
};

#endif  // PLAYER_H
