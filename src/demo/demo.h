#ifndef DEMO_DEMO_H
#define DEMO_DEMO_H

#include "base/closure.h"
#include "engine/animator.h"
#include "engine/font.h"
#include "engine/game.h"
#include "engine/persistent_data.h"
#include "engine/solid_quad.h"
#include "engine/sound_player.h"

#include "demo/credits.h"
#include "demo/enemy.h"
#include "demo/hud.h"
#include "demo/menu.h"
#include "demo/player.h"
#include "demo/sky_quad.h"

// #define LOAD_TEST

class Demo final : public eng::Game {
 public:
  Demo();
  ~Demo() final;

  bool Initialize() final;

  void Update(float delta_time) final;

  void ContextLost() final {}

  void LostFocus() final;

  void GainedFocus(bool from_interstitial_ad) final;

  void AddScore(size_t score);

  void SetEnableMusic(bool enable);

  void EnterMenuState();
  void EnterCreditsState();
  void EnterGameState();
  void EnterGameOverState();

  const eng::Font& GetFont() { return font_; }

  Player& GetPlayer() { return player_; }
  Enemy& GetEnemy() { return enemy_; }

  int wave() const { return wave_; }

  size_t GetHighScore() const;

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

  size_t wave_score_ = 0;
  size_t total_score_ = 0;
  size_t delta_score_ = 0;

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

  bool do_benchmark_ = true;
  float benchmark_time_ = 0;
  int num_benchmark_samples_ = 0;
  int avarage_fps_ = 0;

  void UpdateMenuState(float delta_time);
  void UpdateGameState(float delta_time);

  void Continue();
  void StartNewGame();

  void StartNextStage(bool boss);

  void Win();

  void Dimmer(bool enable);

  void SetDelayedWork(float seconds, base::Closure cb);

  void BenchmarkResult(int avarage_fps);
};

#endif  // DEMO_DEMO_H
