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
  if (!bg_.Create("star-blasts.jpg", Vector2(0, 0))) {
    LOG << "Failed to create the backgroud.";
    return false;
  }
  bg_.SetScale(ToScale(256, 256));

  if (!sprite_.Create("spaceship.png", Vector2(0, 0))) {
    LOG << "Failed to create the sprite.";
    return false;
  }
  sprite_.SetScale(ToScale(50, 50));

  return true;
}

void Demo::Update(float delta_time) {
  seconds_accumulated_ += delta_time;

  std::vector<std::string> lines;
  std::string line = "frames dropped: ";
  line += std::to_string(engine::Engine::Get().GetRenderer().num_frames_dropped());
  lines.push_back(line);
  line = "global commands: ";
  line += std::to_string(engine::Engine::Get().GetRenderer().num_global_commands());
  lines.push_back(line);
  line = "render queue: ";
  line += std::to_string(engine::Engine::Get().GetRenderer().max_render_queue_size());
  lines.push_back(line);
  if (!stats_.Print(lines, 300, Vector2(0, 0))) {
    LOG << "Failed to create the text.";
  }
}

void Demo::Draw(float frame_frac) {
  engine::Engine::Get().GetRenderer().EnableBlend();

  float scale_x = bg_.GetScale().x;
  float scale_y = bg_.GetScale().y;
  float scroll_offset_y = fmod(-seconds_accumulated_ * 0.15f, scale_y);
  for (float y = -1.0f + scale_y / 2; y <= 1.0f + scale_y; y += scale_y) {
    for (float x = -1.0f + scale_x / 2; x <= 1.0f; x += scale_x) {
      bg_.Draw(Vector2(x, y + scroll_offset_y));
    }
  }

  stats_.Draw(Vector2(0, 0));
  sprite_.Draw(Vector2(0.5f, 0.5f));
}

Vector2 Demo::ToScale(int width, int height) {
  float horizontal_ratio =
      (float)width / engine::Engine::Get().GetRenderer().GetScreenWidth();
  float vertical_ratio =
      (float)height / engine::Engine::Get().GetRenderer().GetScreenHeight();

  // The orthogonal viewport is (-1.0 .. 1.0) x (-1.0 .. 1.0).
  return Vector2(horizontal_ratio * 2.0f, vertical_ratio * 2.0f);
}
