#ifndef DEMO_H
#define DEMO_H

#include "../base/closure.h"
#include "../engine/game.h"
#include "credits.h"
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

  void LostFocus() override;

  void GainedFocus() override;

  void AddScore(int score);

  void EnterMenuState();
  void EnterCreditsState();
  void EnterGameState();

  Player& GetPlayer() { return player_; }
  Enemy& GetEnemy() { return enemy_; }

  int wave() { return wave_; }

 private:
  enum State { kState_Invalid = -1, kMenu, kGame, kCredits, kState_Max };

  State state_ = kState_Invalid;

  Player player_;
  Enemy enemy_;
  Hud hud_;
  Menu menu_;
  Credits credits_;

  SkyQuad sky_;
  int last_dominant_channel_ = -1;

  int score_ = 0;
  int add_score_ = 0;

  int wave_ = 0;

  int last_num_enemies_killed_ = -1;
  int total_enemies_ = 0;

  int waiting_for_next_wave_ = false;

  float delayed_work_timer_ = 0;
  base::Closure delayed_work_cb_;

  void UpdateMenuState(float delta_time);
  void UpdateGameState(float delta_time);

  void Continue();
  void StartNewGame();

  void SetDelayedWork(float seconds, base::Closure cb);
};

#endif  // DEMO_H
