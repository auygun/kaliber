#ifndef DEMO_H
#define DEMO_H

#include <vector>
#include <memory>
#include "background.h"
#include "enemy.h"
#include "player.h"
#include "../engine/game.h"
#include "../engine/image_quad.h"

class Demo : public engine::Game {
 public:
  Demo() = default;
  ~Demo() override = default;

  bool Initialize() override;

  void Update(float delta_time) override;

  Player& GetPlayer() { return player_; }
  Enemy& GetEnemy() { return enemy_; }

 private:
  Background bg_;
  Player player_;
  Enemy enemy_;
};

#endif  // DEMO_H
