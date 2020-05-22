#include "demo.h"

#include <string>

#include "../base/log.h"
#include "../engine/engine.h"
#include "../engine/game_factory.h"
#include "../engine/input_event.h"

DECLARE_GAME_BEGIN
DECLARE_GAME(Demo)
DECLARE_GAME_END

namespace {

}  // namespace

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

  return true;
}

void Demo::Update(float delta_time) {
  eng::Engine& engine = eng::Engine::Get();

  while (std::unique_ptr<eng::InputEvent> event = engine.GetNextInputEvent()) {
    if (event) {
      if (event->GetType() == eng::InputEvent::kDragStart ||
          event->GetType() == eng::InputEvent::kDrag ||
          event->GetType() == eng::InputEvent::kDragEnd ||
          event->GetType() == eng::InputEvent::kDragCancel)
        player_.OnInputEvent(std::move(event));
      else if (event->GetType() == eng::InputEvent::kKeyPress)
        LOG << "Key press: " << std::string({event->GetKeyPress()});
    }
  }

  sky_.Translate({0, delta_time * -0.04f});

  if (add_score_ > 0) {
    score_ += add_score_;
    add_score_ = 0;
    hud_.PrintScore(score_, true);
  }

  if (enemy_.num_enemies_killed() != last_num_enemies_killed_) {
    last_num_enemies_killed_ = enemy_.num_enemies_killed();
    int enemies_remaining = total_enemies_ - last_num_enemies_killed_;

    float progress = 1;
    if (enemies_remaining <= 0) {
      last_num_enemies_killed_ = 0;

      ++wave_;
      float factor = 3 * (log10(5 * (float)wave_) / log10(1.2f)) - 25;
      total_enemies_ = (int)(6 * factor);
      LOG << "wave: " << wave_ << " total_enemies_: " << total_enemies_;

      enemy_.OnWaveChange(wave_);

      hud_.PrintWave(wave_);
    } else {
      progress = (float)enemies_remaining / (float)total_enemies_; //kNumEnemiesPerWave[wave_];
    }
    hud_.SetProgress(progress);
  }

  player_.Update(delta_time);
  enemy_.Update(delta_time);
  hud_.Update(delta_time);
}

void Demo::Draw(float frame_frac) {
  sky_.Draw(frame_frac);
  player_.Draw(frame_frac);
  enemy_.Draw(frame_frac);
  hud_.Draw();
}

void Demo::ContextLost() {
  enemy_.ContextLost();
  player_.ContextLost();
  hud_.ContextLost();
  sky_.ContextLost();
  sky_.Create();
}

void Demo::AddScore(int score) {
  add_score_ += score;
}
