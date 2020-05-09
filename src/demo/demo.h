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

  Player& GetPlayer() { return player_; }
  Enemy& GetEnemy() { return enemy_; }

 private:
  Player player_;
  Enemy enemy_;

  SkyQuad sky_;

  eng::ImageQuad hud_;
  eng::ColorAnimator hud_animator_;

  void UpdateHud(float delta_time);
};

#endif  // DEMO_H
