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
  auto image = std::make_unique<Image>();
  if (!image->Load("star-blasts.jpg"))
    return false;
  if (!bg_.Create(std::move(image))) {
    LOG << "Failed to create the backgroud.";
    return false;
  }
  bg_.Scale(engine::Engine::Get().ToScale(256, 256));

  image = std::make_unique<Image>();
  if (!image->Load("spaceship.png"))
    return false;
  if (!ship_.Create(std::move(image))) {
    LOG << "Failed to create the sprite.";
    return false;
  }
  ship_.Scale(engine::Engine::Get().ToScale(50, 50));

  image = std::make_unique<Image>();
  if (!image->Load("enemy.png"))
    return false;
  if (!enemy_.Create(std::move(image))) {
    LOG << "Failed to create the sprite.";
    return false;
  }

  return true;
}

void Demo::Update(float delta_time) {
  seconds_accumulated_ += delta_time;
}

void Demo::Draw(float frame_frac) {
  engine::Engine::Get().GetRenderer().EnableBlend();

  float scale_x = bg_.scale().x;
  float scale_y = bg_.scale().y;
  float scroll_offset_y = fmod(-seconds_accumulated_ * 0.15f, scale_y);
  for (float y = -1.0f + scale_y / 2; y <= 1.0f + scale_y; y += scale_y) {
    for (float x = -1.0f + scale_x / 2; x <= 1.0f; x += scale_x) {
      bg_.Translate(Vector2(x, y + scroll_offset_y));
      bg_.Draw();
    }
  }

  ship_.Translate(Vector2(0, -0.5f));
  ship_.Draw();

  enemy_.Translate(Vector2(0.5f, 0.5f));
  enemy_.Draw();
}
