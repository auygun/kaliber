#ifndef DEMO_H
#define DEMO_H

#include "../base/closure.h"
#include "../engine/animator.h"
#include "../engine/font.h"
#include "../engine/game.h"
#include "../engine/persistent_data.h"
#include "../engine/solid_quad.h"
#include "../engine/sound_player.h"
#include "credits.h"
#include "enemy.h"
#include "hud.h"
#include "menu.h"
#include "player.h"
#include "sky_quad.h"

class Demo : public eng::Game {
 public:
  Demo();
  ~Demo() override;

  bool Initialize() override;

  void Update(float delta_time) override;

  void ContextLost() override;

  void LostFocus() override;

  void GainedFocus(bool from_interstitial_ad) override;

  void AddScore(int score);

  void SetEnableMusic(bool enable);

  void EnterMenuState();
  void EnterCreditsState();
  void EnterGameState();
  void EnterGameOverState();

  const eng::Font& GetFont() { return font_; }

  Player& GetPlayer() { return player_; }
  Enemy& GetEnemy() { return enemy_; }

  int wave() const { return wave_; }

  int GetHighScore() const;

  float stage_time() const { return stage_time_; }

  eng::PersistentData& saved_data() { return saved_data_; }
  const eng::PersistentData& saved_data() const { return saved_data_; }

 private:
  enum State {
    kState_Invalid = -1,
    kMenu,
    kGame,
    kCredits,
    kGameOver,
    kState_Max
  };

  State state_ = kState_Invalid;

  Player player_;
  Enemy enemy_;
  Hud hud_;
  Menu menu_;
  Credits credits_;

  SkyQuad sky_;
  int last_dominant_channel_ = -1;

  eng::Font font_;

  int wave_score_ = 0;
  int total_score_ = 0;
  int delta_score_ = 0;

  int wave_ = 0;

  int last_num_enemies_killed_ = -1;
  int total_enemies_ = 0;

  int waiting_for_next_wave_ = false;

  bool boss_fight_ = false;

  float stage_time_ = 0;

  eng::SoundPlayer music_;
  eng::SoundPlayer boss_music_;

  eng::SolidQuad dimmer_;
  eng::Animator dimmer_animator_;
  bool dimmer_active_ = false;

  float delayed_work_timer_ = 0;
  base::Closure delayed_work_cb_;

  eng::PersistentData saved_data_;

  void UpdateMenuState(float delta_time);
  void UpdateGameState(float delta_time);

  void Continue();
  void StartNewGame();

  void StartNextStage(bool boss);

  void Dimmer(bool enable);

  void SetDelayedWork(float seconds, base::Closure cb);

  int DoBenchmark();
  void BenchmarkResult(int avarage_fps);
};

#endif  // DEMO_H
