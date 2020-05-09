#ifndef PLAYER_H
#define PLAYER_H

#include "../base/vecmath.h"
#include "damage_type.h"
#include "../engine/image_quad.h"
#include "../engine/color_animator.h"
#include "../engine/frame_animator.h"
#include "../engine/draw_animator.h"

namespace eng {
class InputEvent;
} //  namespace eng

class Player {
 public:
  Player() = default;
  ~Player() = default;

  bool Initialize();

  void ContextLost();

  void Update(float delta_time);

  void OnInputEvent(std::unique_ptr<eng::InputEvent> event);

  Vector2 GetWeaponPos(DamageType type) const;

 private:
  eng::ImageQuad drag_sign_[2];
  eng::ImageQuad weapon_[2];
  eng::ImageQuad beam_[2];
  eng::ImageQuad beam_spark_[2];

  eng::DrawAnimator weapon_rotate_animator_;
  eng::FrameAnimator weapon_animator_[2];
  eng::ColorAnimator beam_animator_[2];
  eng::DrawAnimator beam_spark_animator_[2];

  DamageType active_weapon_ = kDamageType_Invalid;

  Vector2 drag_start_ = {0, 0};
  Vector2 drag_end_ = {0, 0};

  DamageType GetWeaponType(const Vector2& pos);

  void SetBeamLength(DamageType type, float len);

  void Fire(DamageType type);
  bool IsFiring(DamageType type);

  void SetupWeapons();

  void DragStart(const Vector2& pos);
  void Drag(const Vector2& pos);
  void DragEnd();
  bool ValidateDrag();
};

#endif  // PLAYER_H
