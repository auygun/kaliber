#include "demo.h"
#include "../base/log.h"
#include "../base/image.h"
#include "../engine/engine.h"
#include "../engine/game_factory.h"
#include <sstream>
#include <stdio.h>
#include <string>
#include <set>

DECLARE_GAME_BEGIN
  DECLARE_GAME(Demo)
DECLARE_GAME_END

#define FULLSCREEN_TEST

Demo::Demo() : config_{1280, 1024} {}

bool Demo::Initialize() {
  if (!quad_.Create())
    return false;

  Image image;
  if (!image.Load("stock-1.jpg"))
    return false;
  if (!texture_.Create(image))
    return false;

  // Ddetermine the quad scale.
  constexpr int quad_size = 50;
  float horizontal_ratio = (float)config_.screen_width / quad_size;
  float vertical_ratio = (float)config_.screen_height / quad_size;

  // The orthogonal viewport is (-1.0 .. 1.0) x (-1.0 .. 1.0).
  scale_.x = 2.0f / horizontal_ratio;
  scale_.y = 2.0f / vertical_ratio;
  LOG("scale_: %f %f\n", scale_.x, scale_.y);

  return true;
}

void Demo::Shutdown() {
}

void Demo::Update(float delta_time) {
}

void Demo::Draw(float frame_frac) {
  engine::Engine::Get().GetRenderer().EnableAlphaBlending();
  texture_.Activate();
  quad_.Activate();
  quad_.Draw(Vector2(0, 0), scale_, Vector3(1, 1, 1));
  quad_.Draw(Vector2(0.5f, 0.5f), scale_, Vector3(1, 1, 1));
}
