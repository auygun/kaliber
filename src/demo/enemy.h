#ifndef ENEMY_H
#define ENEMY_H

#include "damage_type.h"
#include "../engine/image_quad.h"
#include "../engine/solid_quad.h"
#include "../engine/animator.h"
#include <list>
#include <memory>

namespace eng {
class Image;
class Font;
}

class Enemy {
 public:
  Enemy() = default;
  ~Enemy() = default;

  bool Initialize();

  void ContextLost();

  void Update(float delta_time);

  void Draw(float frame_frac);

  bool HasTarget(DamageType damage_type);
  Vector2 GetTargetPos(DamageType damage_type);

  void SelectTarget(DamageType damage_type,
                    const Vector2& weapon_pos,
                    const Vector2& target_pos);
  void DeselectTarget(DamageType damage_type);
  void HitTarget(DamageType damage_type);

  void ResetNumEnemiesKilled() { num_enemies_killed_ = 0; }

  int num_enemies_killed() { return num_enemies_killed_; }

 private:
  enum UnitType {
    kUnitType_Invalid = -1,
    kUnitType_Skull,
    kUnitType_Bug,
    kUnitType_Tank,
    kUnitType_Max
  };

  struct Unit {
    UnitType unit_type = kUnitType_Invalid;
    DamageType damage_type = kDamageType_Invalid;

    bool marked_for_removal = false;
    DamageType targetted_by_weapon_ = kDamageType_Invalid;
    int total_health = 0;
    int hit_points = 0;

    eng::ImageQuad sprite;
    eng::ImageQuad target;
    eng::ImageQuad blast;
    eng::ImageQuad score;
    eng::SolidQuad health_base;
    eng::SolidQuad health_bar;

    eng::Animator movement_animator;
    eng::Animator sprite_animator;
    eng::Animator target_animator;
    eng::Animator blast_animator;
    eng::Animator health_animator;
    eng::Animator score_animator;
  };

  std::shared_ptr<const eng::Image> skull_frames_;
  std::shared_ptr<const eng::Image> tank_frames_;
  std::shared_ptr<const eng::Image> bug_frames_;
  std::shared_ptr<const eng::Image> target_frames_;
  std::shared_ptr<const eng::Image> blast_frames_;

  std::shared_ptr<eng::Font> font_;

  std::list<Unit> enemies_;
  float seconds_since_last_spawn_ = 0;

  int num_enemies_killed_ = 0;

  void Spawn(UnitType unit_type,
             DamageType damage_type,
             const Vector2& pos,
             float speed);

  Unit* GetTarget(DamageType damage_type);

  int GetScore(UnitType unit_type);
};

#endif  // ENEMY_H
