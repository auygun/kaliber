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
}
