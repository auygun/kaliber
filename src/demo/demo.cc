#include "demo.h"
#include "../base/log.h"
#include "../base/image.h"
#include "../engine/engine.h"
#include "../engine/game_factory.h"
#include <sstream>
#include <stdio.h>
#include <string>
#include <set>
#include <math.h>

DECLARE_GAME_BEGIN
  DECLARE_GAME(Demo)
DECLARE_GAME_END

bool Demo::Initialize() {
  Vector2 scale = ToScale(256, 256);
  if (!bg_tiles_.Create("star-blasts.jpg", Vector2(0, 0), scale)) {
    LOG("Failed to create the backgroud.");
    return false;
  }

  scale = ToScale(50, 50);
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
  secondsAccumulated += delta_time;
  // LOG("%f\n", secondsAccumulated);
}

void Demo::Draw(float frame_frac) {
  engine::Engine::Get().GetRenderer().EnableBlend();

  float scroll_offset_y = fmod(-secondsAccumulated * 0.15f, bg_tiles_.GetScale().y);
  for (float y = -1.0f; y <= 1.0f + bg_tiles_.GetScale().y; y += bg_tiles_.GetScale().y) {
    for (float x = -1.0f; x <= 1.0f + bg_tiles_.GetScale().x; x += bg_tiles_.GetScale().x) {
      bg_tiles_.Draw(Vector2(x, y + scroll_offset_y));
    }
  }

  sprite_.Draw(Vector2(0, 0));
  sprite_.Draw(Vector2(0.5f, 0.5f));
}

Vector2 Demo::ToScale(int width, int height) {
  float horizontal_ratio =
    (float)width / engine::Engine::Get().GetRenderer().GetScreenWidth();
  float vertical_ratio =
    (float)height/ engine::Engine::Get().GetRenderer().GetScreenHeight();

  // The orthogonal viewport is (-1.0 .. 1.0) x (-1.0 .. 1.0).
  return Vector2(horizontal_ratio * 2.0f, vertical_ratio * 2.0f);
}
