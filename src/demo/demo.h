#ifndef DEMO_H
#define DEMO_H

#include <vector>
#include <memory>
#include "enemy.h"
#include "player.h"
#include "hud.h"
#include "sky_quad.h"
#include "../engine/game.h"
#include "../engine/image_quad.h"
#include "../engine/animator.h"

class Demo : public eng::Game {
 public:
  Demo() = default;
  ~Demo() override = default;

  bool Initialize() override;

  void Update(float delta_time) override;

  void Draw(float frame_frac) override;

  void ContextLost() override;

  void AddScore(int score);

  Player& GetPlayer() { return player_; }
  Enemy& GetEnemy() { return enemy_; }

 private:
  Player player_;
  Enemy enemy_;
  Hud hud_;

  SkyQuad sky_;

  int score_ = 0;
  int add_score_ = 0;

  int wave_ = 1;

  int last_num_enemies_killed_ = 0;
};

#endif  // DEMO_H
