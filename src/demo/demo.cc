#include "demo.h"

#include <algorithm>
#include <string>

#include "../base/interpolation.h"
#include "../base/log.h"
#include "../base/random.h"
#include "../engine/engine.h"
#include "../engine/game_factory.h"
#include "../engine/input_event.h"

DECLARE_GAME_BEGIN
DECLARE_GAME(Demo)
DECLARE_GAME_END

using namespace base;
using namespace eng;

bool Demo::Initialize() {
  if (!font_.Load("PixelCaps!.ttf"))
    return false;

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

  if (!credits_.Initialize()) {
    LOG << "Failed to create the credits.";
    return false;
  }

  EnterMenuState();

  return true;
}

void Demo::Update(float delta_time) {
  Engine& engine = Engine::Get();

  while (std::unique_ptr<InputEvent> event = engine.GetNextInputEvent()) {
    if (state_ == kMenu)
      menu_.OnInputEvent(std::move(event));
    else if (state_ == kCredits)
      credits_.OnInputEvent(std::move(event));
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
    hud_.SetScore(score_, true);
  }

  sky_.Update(delta_time);
  player_.Update(delta_time);
  enemy_.Update(delta_time);

  if (state_ == kMenu)
    UpdateMenuState(delta_time);
  else if (state_ == kGame)
    UpdateGameState(delta_time);
}

void Demo::ContextLost() {
  sky_.ContextLost();
}

void Demo::LostFocus() {
  if (state_ == kGame)
    EnterMenuState();
}

void Demo::GainedFocus(bool from_interstitial_ad) {}

void Demo::AddScore(int score) {
  add_score_ += score;
}

void Demo::EnterMenuState() {
  if (state_ == kMenu)
    return;

  if (state_ == kState_Invalid || state_ == kGame) {
    sky_.SetSpeed(0);
    player_.Pause(true);
    enemy_.Pause(true);
  }

  if (wave_ == 0) {
    menu_.SetOptionEnabled(Menu::kContinue, false);
  } else {
    menu_.SetOptionEnabled(Menu::kContinue, true);
    menu_.SetOptionEnabled(Menu::kNewGame, false);
  }
  menu_.Show();
  state_ = kMenu;
}

void Demo::EnterCreditsState() {
  if (state_ == kCredits)
    return;
  credits_.Show();
  state_ = kCredits;
}

void Demo::EnterGameState() {
  if (state_ == kGame)
    return;

  sky_.SetSpeed(0.04f);
  hud_.Show();
  player_.Pause(false);
  enemy_.Pause(false);
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
      menu_.Hide();
      EnterCreditsState();
      break;
    case Menu::kExit:
      Engine::Get().Exit();
      break;
    default:
      NOTREACHED << "- Unknown menu option: " << menu_.selected_option();
  }
}

void Demo::UpdateGameState(float delta_time) {
  if (waiting_for_next_wave_)
    return;

  if (enemy_.num_enemies_killed_in_current_wave() != last_num_enemies_killed_) {
    last_num_enemies_killed_ = enemy_.num_enemies_killed_in_current_wave();
    int enemies_remaining = total_enemies_ - last_num_enemies_killed_;

    if (enemies_remaining <= 0) {
      waiting_for_next_wave_ = true;
      hud_.SetProgress(wave_ > 0 ? 0 : 1);

      enemy_.OnWaveFinished();

      SetDelayedWork(1, [&]() -> void {
        Random& rnd = Engine::Get().GetRandomGenerator();
        int dominant_channel = rnd.Roll(3) - 1;
        if (dominant_channel == last_dominant_channel_)
          dominant_channel = (dominant_channel + 1) % 3;
        last_dominant_channel_ = dominant_channel;

        float weights[3] = {0, 0, 0};
        weights[dominant_channel] = 1;
        Vector4 c = {Lerp(0.75f, 0.95f, rnd.GetFloat()) * weights[0],
                     Lerp(0.75f, 0.95f, rnd.GetFloat()) * weights[1],
                     Lerp(0.75f, 0.95f, rnd.GetFloat()) * weights[2], 1};
        c += {Lerp(0.1f, 0.5f, rnd.GetFloat()) * (1 - weights[0]),
              Lerp(0.1f, 0.5f, rnd.GetFloat()) * (1 - weights[1]),
              Lerp(0.1f, 0.5f, rnd.GetFloat()) * (1 - weights[2]), 1};
        sky_.SwitchColor(c);

        ++wave_;
        hud_.SetScore(score_, true);
        hud_.SetWave(wave_, true);
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
  DCHECK(delayed_work_cb_ == nullptr);
  delayed_work_cb_ = std::move(cb);
  delayed_work_timer_ = seconds;
}
