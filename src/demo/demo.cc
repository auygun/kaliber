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

  hud_.SetVisible(true);
  engine.AddDrawable(&hud_);

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

  constexpr float horizontal_margin = 0.03f;
  constexpr float vertical_margin = 0.02f;

  int width = engine.GetScreenWidth() -
      engine.GetScreenWidth() * horizontal_margin * 2;
  auto image = std::make_shared<eng::Image>();
  image->Create(width, engine.GetFont().GetLineHeight());
  float c[4] = {0.82f, 0.593, 0.088, 0};
  image->Clear(c);

  engine.GetFont().Print(0, 0, "12345", image->GetBuffer(), image->GetWidth());

  hud_.Create(image);
  hud_.AutoScale();

  Vector2 pos = (engine.GetScreenSize() / 2 - hud_.GetScale() / 2);
  pos -= engine.GetScreenSize() * Vector2(horizontal_margin, vertical_margin);
  hud_.SetOffset(pos * Vector2(-1, 1));
}
