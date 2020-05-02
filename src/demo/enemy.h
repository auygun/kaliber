#ifndef ENEMY_H
#define ENEMY_H

#include "game_object.h"
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

  void TryTarget(const Vector2& weapon_pos, const Vector2& target_pos);

 private:
  enum Type { kGreem, kBlue };

  struct EnemyTraits {
    Type type;
    bool alive = true;
    bool targetted = false;
    engine::ImageQuad sprite;
    engine::ImageQuad target;
    engine::DrawAnimator draw_animator;
    engine::FrameAnimator sprite_frame_animator;
    engine::FrameAnimator target_frame_animator;
  };

  std::shared_ptr<const engine::Image> enemy_frames_;
  std::shared_ptr<const engine::Image> target_frames_;

  std::list<EnemyTraits> enemies_;
  float seconds_since_last_spawn_ = 0;

  void Spawn(Type type, const Vector2& pos, float speed);
};

#endif  // ENEMY_H
