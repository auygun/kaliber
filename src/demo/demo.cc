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

  if (!beam_.Initialize()) {
    LOG << "Failed to create the enemy.";
    return false;
  }

  return true;
}

void Demo::Update(float delta_time) {
  bg_.Update(delta_time);
  enemy_.Update(delta_time);
  beam_.Update(delta_time);
}
