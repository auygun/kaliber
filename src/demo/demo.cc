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
  ship_.Translate(Vector2(0, -0.5f));
  ship_.Scale(engine::Engine::Get().ToScale(50, 50));
  engine::Engine::Get().AddDrawable(&ship_);

  image = engine.GetAssetManager().GetImage("enemy.png");
  if (!enemy_.Create(image)) {
    LOG << "Failed to create the sprite.";
    return false;
  }
  enemy_.Translate(Vector2(0.5f, 0.5f));
  engine::Engine::Get().AddDrawable(&enemy_);

  return true;
}

void Demo::Update(float delta_time) {
  seconds_accumulated_ += delta_time;
  bg_.Update(delta_time);
}
