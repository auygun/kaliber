#ifndef ENEMY_H
#define ENEMY_H

#include "damage_type.h"
#include "../engine/image_quad.h"
#include "../engine/draw_animator.h"
#include "../engine/frame_animator.h"
#include <list>
#include <memory>

class Enemy {
 public:
  Enemy() = default;
  ~Enemy() = default;

  bool Initialize();

  void Update(float delta_time);

  bool HasTarget(DamageType damage_type);
  Vector2 GetTargetPos(DamageType damage_type);

  void SelectTarget(DamageType damage_type,
                    const Vector2& weapon_pos,
                    const Vector2& target_pos);
  void DeselectTarget(DamageType damage_type);
  void KillTarget(DamageType damage_type);

 private:
  enum UnitType {
    kUnitType_Invalid = -1,
    kUnitType_Skull,
    kUnitType_Bug,
    kUnitType_Max
  };

  struct Unit {
    UnitType unit_type = kUnitType_Invalid;
    DamageType damage_type = kDamageType_Invalid;

    bool alive = true;
    bool marked_for_removal = false;
    DamageType targetted_by_weapon_ = kDamageType_Invalid;

    engine::ImageQuad sprite;
    engine::ImageQuad target;
    engine::ImageQuad blast;

    engine::DrawAnimator draw_animator;
    engine::FrameAnimator sprite_frame_animator;
    engine::FrameAnimator target_frame_animator;
    engine::FrameAnimator blast_frame_animator;
  };

  std::shared_ptr<const engine::Image> skull_frames_;
  std::shared_ptr<const engine::Image> bug_frames_;
  std::shared_ptr<const engine::Image> target_frames_;
  std::shared_ptr<const engine::Image> blast_frames_;

  std::list<Unit> enemies_;
  float seconds_since_last_spawn_ = 0;

  void Spawn(UnitType unit_type,
             DamageType damage_type,
             const Vector2& pos,
             float speed);

  Unit* GetTarget(DamageType damage_type);
};

#endif  // ENEMY_H
