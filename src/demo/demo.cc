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
  engine::Engine& engine = engine::Engine::Get();

  bg_.Initialize();

  auto image = engine.GetAssetManager().GetImage("spaceship.png");
  if (!image)
    return false;
  if (!ship_.Create(image)) {
    LOG << "Failed to create the sprite.";
    return false;
  }
  ship_.SetVisible(true);
  ship_.Scale(0.4f);
  ship_.SetOffset(Vector2(0, -0.5f));
  engine::Engine::Get().AddDrawable(&ship_);

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
  engine::Engine& engine = engine::Engine::Get();

  while (std::unique_ptr<engine::InputEvent> event = engine.GetNextInputEvent()) {
    if (event) {
      if (event->GetEventType() == engine::InputEvent::kDragStart ||
          event->GetEventType() == engine::InputEvent::kDrag ||
          event->GetEventType() == engine::InputEvent::kDragEnd)
        player_.OnInputEvent(std::move(event));
    }
  }

  bg_.Update(delta_time);
  enemy_.Update(delta_time);
  player_.Update(delta_time);
}
