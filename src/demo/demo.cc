#include "demo.h"

#include <algorithm>
#include <string>

#include "../base/interpolation.h"
#include "../base/random_generator.h"
#include "../base/log.h"
#include "../engine/engine.h"
#include "../engine/game_factory.h"
#include "../engine/input_event.h"

DECLARE_GAME_BEGIN
DECLARE_GAME(Demo)
DECLARE_GAME_END

using namespace base;
using namespace eng;

bool Demo::Initialize() {
  if (!sky_.Create()) {
    LOG << "Could not create the sky.";
    return false;
  }

  if (!enemy_.Initialize()) {
    LOG << "Failed to create the enemy.";
    return false;
  }

  if (!player_.Initialize()) {
    LOG << "Failed to create the enemy.";
    return false;
  }

  if (!hud_.Initialize()) {
    LOG << "Failed to create the hud.";
    return false;
  }

  if (!menu_.Initialize()) {
    LOG << "Failed to create the menu.";
    return false;
  }

  EnterMenuState();

  return true;
}

void Demo::Update(float delta_time) {
  eng::Engine& engine = eng::Engine::Get();

  while (std::unique_ptr<eng::InputEvent> event = engine.GetNextInputEvent()) {
    if (state_ == kMenu)
      menu_.OnInputEvent(std::move(event));
    else
      player_.OnInputEvent(std::move(event));
  }

  if (delayed_work_timer_ > 0) {
    delayed_work_timer_ -= delta_time;
    if (delayed_work_timer_ <= 0) {
      base::Closure cb = std::move(delayed_work_cb_);
      delayed_work_cb_ = nullptr;
      cb();
    }
  }

  if (add_score_ > 0) {
    score_ += add_score_;
    add_score_ = 0;
    hud_.PrintScore(score_, true);
  }

  hud_.Update(delta_time);
  menu_.Update(delta_time);

  if (state_ == kMenu)
      UpdateMenuState(delta_time);
  else
      UpdateGameState(delta_time);
}

void Demo::Draw(float frame_frac) {
  sky_.Draw(frame_frac);
  player_.Draw(frame_frac);
  enemy_.Draw(frame_frac);
  hud_.Draw();
  menu_.Draw();
}

void Demo::ContextLost() {
  enemy_.ContextLost();
  player_.ContextLost();
  hud_.ContextLost();
  menu_.ContextLost();
  sky_.ContextLost();
}

void Demo::LostFocus() {
  EnterMenuState();
}

void Demo::GainedFocus() {
}

void Demo::AddScore(int score) {
  add_score_ += score;
}

void Demo::EnterMenuState() {
  if (state_ == kMenu)
    return;
  if (wave_ == 0) {
    menu_.SetOptionEnabled(Menu::kContinue, false);
  } else {
    menu_.SetOptionEnabled(Menu::kContinue, true);
    menu_.SetOptionEnabled(Menu::kNewGame, false);
  }
  menu_.Show();
  state_ = kMenu;
}

void Demo::EnterGameState() {
  if (state_ == kGame)
    return;
  state_ = kGame;
}

void Demo::UpdateMenuState(float delta_time) {
  switch (menu_.selected_option()) {
    case Menu::kOption_Invalid:
      break;
    case Menu::kContinue:
      menu_.Hide();
      Continue();
      break;
    case Menu::kNewGame:
      menu_.Hide();
      StartNewGame();
      break;
    case Menu::kCredits:
      break;
    case Menu::kExit:
      Engine::Get().Exit();
      break;
    default:
      assert(false);
  }
}

void Demo::UpdateGameState(float delta_time) {
  sky_.Update(delta_time);
  player_.Update(delta_time);
  enemy_.Update(delta_time);

  if (waiting_for_next_wave_)
    return;

  if (enemy_.num_enemies_killed_in_current_wave() != last_num_enemies_killed_) {
    last_num_enemies_killed_ = enemy_.num_enemies_killed_in_current_wave();
    int enemies_remaining = total_enemies_ - last_num_enemies_killed_;

    if (enemies_remaining <= 0) {
      waiting_for_next_wave_ = true;
      hud_.SetProgress(0);

      enemy_.OnWaveFinished();

      SetDelayedWork(1, [&]() -> void {
        RandomGenerator& rnd = eng::Engine::Get().GetRandomGenerator();
        int dominant_channel = rnd.GetInt() % 3;
        float weights[3] = {0 ,0 ,0};
        weights[dominant_channel] = 1;
        Vector4 c = {Lerp(0.80f, 0.95f, rnd.GetFloat()) * weights[0],
                     Lerp(0.80f, 0.95f, rnd.GetFloat()) * weights[1],
                     Lerp(0.80f, 0.95f, rnd.GetFloat()) * weights[2], 1};
        c += {Lerp(0.1f, 0.8f, rnd.GetFloat()) * (1 - weights[0]),
              Lerp(0.1f, 0.8f, rnd.GetFloat()) * (1 - weights[1]),
              Lerp(0.1f, 0.8f, rnd.GetFloat()) * (1 - weights[2]), 1};
        sky_.SwitchColor(c);

        ++wave_;
        hud_.PrintScore(score_, false);
        hud_.PrintWave(wave_, true);
        hud_.SetProgress(1);

        float factor = 3 * (log10(5 * (float)wave_) / log10(1.2f)) - 25;
        total_enemies_ = (int)(6 * factor);
        last_num_enemies_killed_ = 0;
        DLOG << "wave: " << wave_ << " total_enemies_: " << total_enemies_;

        enemy_.OnWaveStarted(wave_);

        waiting_for_next_wave_ = false;
      });
    } else {
      hud_.SetProgress((float)enemies_remaining / (float)total_enemies_);
    }
  }
}

void Demo::Continue() {
  EnterGameState();
}

void Demo::StartNewGame() {
  score_ = 0;
  add_score_ = 0;
  wave_ = 0;
  last_num_enemies_killed_ = -1;
  total_enemies_ = 0;
  waiting_for_next_wave_ = false;
  delayed_work_timer_ = 0;
  delayed_work_cb_ = nullptr;
  EnterGameState();
}

void Demo::SetDelayedWork(float seconds, base::Closure cb) {
  assert(delayed_work_cb_ == nullptr);
  delayed_work_cb_ = std::move(cb);
  delayed_work_timer_ = seconds;
}
