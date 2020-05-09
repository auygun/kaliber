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

  sky_.Create();
  sky_.SetVisible(true);
  engine.AddDrawable(&sky_);

  hud_.SetColor({0.895f, 0.692f, 0.24f, 1});
  hud_.SetVisible(true);
  engine.AddDrawable(&hud_);

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
          event->GetEventType() == eng::InputEvent::kDragEnd)
        player_.OnInputEvent(std::move(event));

        hud_animator_.SetTarget({1, 1, 1, 1}, 0.5f);
        hud_animator_.SetCallback([&]()->void {
          hud_animator_.SetCallback(nullptr);
          hud_animator_.SetTarget({0.895f, 0.692f, 0.24f, 1}, 0.5f);
          hud_animator_.Play();
        });
        hud_animator_.Play();
    }
  }

  enemy_.Update(delta_time);
  player_.Update(delta_time);

  UpdateHud(delta_time);
}

void Demo::ContextLost() {
  enemy_.ContextLost();
  player_.ContextLost();
  sky_.Create();
}

void Demo::UpdateHud(float delta_time) {
  eng::Engine& engine = eng::Engine::Get();

  constexpr float horizontal_margin = 0.055f;
  constexpr float vertical_margin = 0.02f;

  int width = engine.GetScreenWidth() -
      engine.GetScreenWidth() * horizontal_margin * 2;
  auto image = std::make_shared<eng::Image>();
  image->Create(width, engine.GetFont().GetLineHeight());
  float c[4] = {1, 1, 1, 0};
  image->Clear(c);

  engine.GetFont().Print(0, 0, "12345", image->GetBuffer(), image->GetWidth());

  hud_.Create(image);
  hud_.AutoScale();

  Vector2 pos = (engine.GetScreenSize() / 2 - hud_.GetScale() / 2);
  pos -= engine.GetScreenSize() * Vector2(horizontal_margin, vertical_margin);
  hud_.SetOffset(pos * Vector2(-1, 1));

  hud_animator_.Update(delta_time);
}
