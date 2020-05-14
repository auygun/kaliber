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
  eng::Engine& engine = eng::Engine::Get();

  if (!font_.Create("PixelCaps!.ttf")) {
    LOG << "Failed to create the font.";
    return false;
  }

  sky_.Create();
  sky_.SetVisible(true);
  engine.AddDrawable(&sky_);

  hud_.SetColor({0.895f, 0.692f, 0.24f, 1});
  hud_.SetVisible(true);
  engine.AddDrawable(&hud_);
  PrintScore(false);

  hud_animator_cb_ = [&]()->void {
    hud_animator_.SetEndCallback(eng::Animator::kBlending, nullptr);
    hud_animator_.SetBlending({0.895f, 0.692f, 0.24f, 1}, 8);
    hud_animator_.Play(eng::Animator::kBlending, false);
  };
  hud_animator_.Attach(&hud_);

  if (!enemy_.Initialize()) {
    LOG << "Failed to create the enemy.";
    return false;
  }

  if (!player_.Initialize()) {
    LOG << "Failed to create the enemy.";
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

  enemy_.Update(delta_time);
  player_.Update(delta_time);

  if (add_score_ > 0) {
    score_ += add_score_;
    add_score_ = 0;
    PrintScore(true);
  }

  hud_animator_.Update(delta_time);
}

void Demo::ContextLost() {
  enemy_.ContextLost();
  player_.ContextLost();
  sky_.Create();
  PrintScore(true);
}

void Demo::AddScore(int score) {
  add_score_ += score;
}

void Demo::PrintScore(bool flash) {
  eng::Engine& engine = eng::Engine::Get();

  constexpr float horizontal_margin = 0.055f;
  constexpr float vertical_margin = 0.02f;

  int width = engine.GetScreenWidth() -
      engine.GetScreenWidth() * horizontal_margin * 2;
  auto image = std::make_shared<eng::Image>();
  image->Create(width, font_.GetLineHeight());
  float c[4] = {1, 1, 1, 0};
  image->Clear(c);

  std::string score_str = std::to_string(score_);
  font_.Print(0, 0, score_str.c_str(), image->GetBuffer(),
      image->GetWidth());

  hud_.Create(image);
  hud_.AutoScale();

  Vector2 pos = (engine.GetScreenSize() / 2 - hud_.GetScale() / 2);
  pos -= engine.GetScreenSize() * Vector2(horizontal_margin, vertical_margin);
  hud_.SetOffset(pos * Vector2(-1, 1));

  if (flash) {
    hud_animator_.SetEndCallback(eng::Animator::kBlending, hud_animator_cb_);
    hud_animator_.SetBlending({1, 1, 1, 1}, 12);
    hud_animator_.Play(eng::Animator::kBlending, false);
  }
}
