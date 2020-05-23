#ifndef DEMO_H
#define DEMO_H

#include "../base/closure.h"
#include "../engine/game.h"
#include "enemy.h"
#include "hud.h"
#include "menu.h"
#include "player.h"
#include "sky_quad.h"

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
  Menu menu_;

  SkyQuad sky_;

  int score_ = 0;
  int add_score_ = 0;

  int wave_ = 0;

  int last_num_enemies_killed_ = -1;
  int total_enemies_ = 0;

  int waiting_for_next_wave_ = false;

  float delyaed_work_timer_ = 0;
  base::Closure delayed_work_cb_;

  void UpdateWaveProgress();

  void SetDelayedWork(float seconds, base::Closure cb);
};

#endif  // DEMO_H
