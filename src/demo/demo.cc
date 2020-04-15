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

bool Demo::Initialize() {
  // Ddetermine the quad scale.
  constexpr int quad_size = 50;
  float horizontal_ratio =
    (float)engine::Engine::Get().GetRenderer().GetScreenWidth() / quad_size;
  float vertical_ratio =
    (float)engine::Engine::Get().GetRenderer().GetScreenHeight() / quad_size;

  // The orthogonal viewport is (-1.0 .. 1.0) x (-1.0 .. 1.0).
  Vector2 scale(2.0f / horizontal_ratio, 2.0f / vertical_ratio);
  LOG("scale_: %f %f\n", scale.x, scale.y);
  if (!sprite_.Create("stock-1.jpg", Vector2(0, 0), scale)) {
    LOG("Failed to create the sprite.");
    return false;
  }

  return true;
}

void Demo::Shutdown() {
}

void Demo::Update(float delta_time) {
}

void Demo::Draw(float frame_frac) {
  engine::Engine::Get().GetRenderer().EnableBlend();
  sprite_.Draw(Vector2(0, 0));
  sprite_.Draw(Vector2(0.5f, 0.5f));
}
