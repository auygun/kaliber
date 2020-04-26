#include "demo.h"
#include <math.h>
#include <stdio.h>
#include <set>
#include <sstream>
#include <string>
#include <vector>
#include "../base/image.h"
#include "../base/log.h"
#include "../engine/engine.h"
#include "../engine/game_factory.h"

DECLARE_GAME_BEGIN
DECLARE_GAME(Demo)
DECLARE_GAME_END

bool Demo::Initialize() {
  bg_.Initialize();

  auto image = std::make_unique<Image>();
  if (!image->Load("spaceship.png"))
    return false;
  if (!ship_.Create(std::move(image))) {
    LOG << "Failed to create the sprite.";
    return false;
  }
  ship_.Translate(Vector2(0, -0.5f));
  ship_.Scale(engine::Engine::Get().ToScale(50, 50));
  engine::Engine::Get().AddDrawable(&ship_);

  image = std::make_unique<Image>();
  if (!image->Load("enemy.png"))
    return false;
  if (!enemy_.Create(std::move(image))) {
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
