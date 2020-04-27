#ifndef ENEMY_H
#define ENEMY_H

#include "game_object.h"
#include "../engine/movie_quad.h"
#include "../engine/frame_animator.h"

class Enemy : public GameObject {
 public:
  Enemy() = default;
  ~Enemy() override = default;

  bool Initialize() override;

  void Update(float delta_time) override;

 private:
  engine::MovieQuad sprite_;
  engine::FrameAnimator frame_animator_;
};

#endif  // ENEMY_H
