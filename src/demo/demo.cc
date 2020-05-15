#include "demo.h"
#include <math.h>
#include <stdio.h>
#include <set>
#include <sstream>
#include <string>
#include <vector>
#include "../engine/asset_manager/image.h"
#include "../base/log.h"
#include "../engine/engine.h"
#include "../engine/game_factory.h"
#include "../engine/input_event.h"

DECLARE_GAME_BEGIN
DECLARE_GAME(Demo)
DECLARE_GAME_END

bool Demo::Initialize() {
  sky_.Create();

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
      if (event->GetEventType() == eng::InputEvent::kDragStart ||
          event->GetEventType() == eng::InputEvent::kDrag ||
          event->GetEventType() == eng::InputEvent::kDragEnd ||
          event->GetEventType() == eng::InputEvent::kDragCancel)
        player_.OnInputEvent(std::move(event));
    }
  }

  player_.Update(delta_time);
  enemy_.Update(delta_time);

  if (add_score_ > 0) {
    score_ += add_score_;
    add_score_ = 0;
    hud_.PrintScore(score_, true);
  }

  if (enemy_.num_enemies_killed() != last_num_enemies_killed_) {
    last_num_enemies_killed_ = enemy_.num_enemies_killed();
    int enemies_remaining_ = 100 - last_num_enemies_killed_;
    float progress = 1;
    if (enemies_remaining_ <= 0) {
      enemy_.ResetNumEnemiesKilled();
      last_num_enemies_killed_ = 0;
      hud_.PrintWave(++wave_);
    } else {
      LOG << enemies_remaining_;
      progress = enemies_remaining_ / 100.0f;
    }
    LOG << progress;
    hud_.SetProgress(progress);
  }

  hud_.Update(delta_time);
}

void Demo::Draw(float frame_frac) {
  sky_.Draw();
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
