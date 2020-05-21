#ifndef DEMO_H
#define DEMO_H

#include "../engine/game.h"
#include "enemy.h"
#include "hud.h"
#include "player.h"
#include "sky_quad.h"

constexpr int kMaxWaves = 5;

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

  int wave() { return wave_; }

 private:
  Player player_;
  Enemy enemy_;
  Hud hud_;

  SkyQuad sky_;

  int score_ = 0;
  int add_score_ = 0;

  int wave_ = 0;

  int last_num_enemies_killed_ = 0;
};

#endif  // DEMO_H
