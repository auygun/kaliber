#ifndef PLAYER_H
#define PLAYER_H

#include <memory>

#include "../base/vecmath.h"
#include "../engine/animator.h"
#include "../engine/image_quad.h"
#include "damage_type.h"

namespace eng {
class InputEvent;
}  //  namespace eng

class Player {
 public:
  Player();
  ~Player();

  bool Initialize();

  void Update(float delta_time);

  void Pause(bool pause);

  void OnInputEvent(std::unique_ptr<eng::InputEvent> event);

  base::Vector2 GetWeaponPos(DamageType type) const;
  base::Vector2 GetWeaponScale() const;

 private:
  eng::ImageQuad drag_sign_[2];
  eng::ImageQuad weapon_[2];
  eng::ImageQuad beam_[2];
  eng::ImageQuad beam_spark_[2];

  eng::Animator warmup_animator_[2];
  eng::Animator cooldown_animator_[2];
  eng::Animator beam_animator_[2];
  eng::Animator spark_animator_[2];

  DamageType active_weapon_ = kDamageType_Invalid;

  base::Vector2 drag_start_ = {0, 0};
  base::Vector2 drag_end_ = {0, 0};
  bool drag_valid_ = false;

  DamageType GetWeaponType(const base::Vector2& pos);

  void WarmupWeapon(DamageType type);
  void CooldownWeapon(DamageType type);

  void Fire(DamageType type, base::Vector2 dir);
  bool IsFiring(DamageType type);

  void SetupWeapons();

  void UpdateTarget();

  void DragStart(const base::Vector2& pos);
  void Drag(const base::Vector2& pos);
  void DragEnd();
  void DragCancel();
  bool ValidateDrag();

  void NavigateBack();

  bool CreateRenderResources();
};

#endif  // PLAYER_H
