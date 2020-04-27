#ifndef ENEMY_H
#define ENEMY_H

#include "game_object.h"
#include "../engine/movie_quad.h"

class Enemy : public GameObject {
 public:
  Enemy() = default;
  ~Enemy() override = default;

  bool Initialize() override;

  void Update(float delta_time) override;

 private:
  float seconds_accumulated_ = 0.0f;
  engine::MovieQuad sprite_;
};

#endif  // ENEMY_H
