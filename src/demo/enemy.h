#ifndef ENEMY_H
#define ENEMY_H

#include "game_object.h"
#include "damage_type.h"
#include "../engine/image_quad.h"
#include "../engine/draw_animator.h"
#include "../engine/frame_animator.h"
#include <list>
#include <memory>

class Enemy : public GameObject {
 public:
  Enemy() = default;
  ~Enemy() override = default;

  bool Initialize() override;

  void Update(float delta_time) override;

  bool HasTarget(DamageType type);
  Vector2 GetTargetPos(DamageType type);

  void SelectTarget(DamageType type,
                    const Vector2& weapon_pos,
                    const Vector2& target_pos);
  void KillTarget(DamageType type);

 private:

  struct EnemyTraits {
    DamageType type = kDamageType_Invalid;
    bool alive = true;
    DamageType targetted_by_weapon_ = kDamageType_Invalid;
    engine::ImageQuad sprite;
    engine::ImageQuad target;
    engine::ImageQuad blast;
    engine::DrawAnimator draw_animator;
    engine::FrameAnimator sprite_frame_animator;
    engine::FrameAnimator target_frame_animator;
    engine::FrameAnimator blast_frame_animator;
  };

  std::shared_ptr<const engine::Image> enemy_frames_;
  std::shared_ptr<const engine::Image> target_frames_;
  std::shared_ptr<const engine::Image> blast_frames_;

  std::list<EnemyTraits> enemies_;
  float seconds_since_last_spawn_ = 0;

  void Spawn(DamageType type, const Vector2& pos, float speed);

  EnemyTraits* GetTarget(DamageType type);
};

#endif  // ENEMY_H
