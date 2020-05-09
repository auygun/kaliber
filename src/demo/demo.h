#ifndef DEMO_H
#define DEMO_H

#include <vector>
#include <memory>
#include "enemy.h"
#include "player.h"
#include "sky_quad.h"
#include "../engine/game.h"
#include "../engine/image_quad.h"
#include "../engine/color_animator.h"

class Demo : public eng::Game {
 public:
  Demo() = default;
  ~Demo() override = default;

  bool Initialize() override;

  void Update(float delta_time) override;

  void ContextLost() override;

  void AddScore(int score);

  Player& GetPlayer() { return player_; }
  Enemy& GetEnemy() { return enemy_; }

 private:
  Player player_;
  Enemy enemy_;

  SkyQuad sky_;

  eng::ImageQuad hud_;
  eng::ColorAnimator hud_animator_;

  int score_ = 0;
  int add_score_ = 0;

  void PrintScore(bool flash);
};

#endif  // DEMO_H
