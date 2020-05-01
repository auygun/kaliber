#ifndef PLAYER_H
#define PLAYER_H

#include "../base/vecmath.h"
#include "game_object.h"
#include "../engine/image_quad.h"
#include "../engine/alpha_animator.h"
#include "../engine/frame_animator.h"
#include "../engine/draw_animator.h"

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

 private:
  engine::ImageQuad drag_sign_[2];
  engine::ImageQuad weapon_[2];
  engine::ImageQuad beam_[2];
  engine::ImageQuad beam_dot_[2];
  engine::ImageQuad beam_spark_[2];

  engine::FrameAnimator weapon_animator_[2];
  engine::AlphaAnimator beam_animator_[2];
  engine::DrawAnimator beam_dot_animator_[2];
  engine::DrawAnimator beam_spark_animator_[2];

  int active_weapon_ = -1;

  Vector2 drag_start_ = {0, 0};
  Vector2 drag_end_ = {0, 0};

  void SetActiveWeapon(const Vector2& pos);

  void SetBeamLength(int i, float len);

  void Fire(int i);
  bool IsFiring(int i);

  void SetupWeapons();
};

#endif  // PLAYER_H
