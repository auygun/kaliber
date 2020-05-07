#ifndef DEMO_H
#define DEMO_H

#include <vector>
#include <memory>
#include "enemy.h"
#include "player.h"
#include "sky_quad.h"
#include "../engine/game.h"
#include "../engine/image_quad.h"

class Demo : public eng::Game {
 public:
  Demo() = default;
  ~Demo() override = default;

  bool Initialize() override;

  void Update(float delta_time) override;

  Player& GetPlayer() { return player_; }
  Enemy& GetEnemy() { return enemy_; }

 private:
  Player player_;
  Enemy enemy_;
  SkyQuad sky_;
};

#endif  // DEMO_H
