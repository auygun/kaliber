#include "background.h"
#include <math.h>
#include <stdio.h>
#include <string>
#include "../engine/asset_manager/image.h"
#include "../base/log.h"
#include "../engine/engine.h"

bool Background::Initialize() {
  engine::Engine& engine = engine::Engine::Get();

  auto image = engine.GetAssetManager().GetImage("star-blasts.jpg");
  if (!image)
    return false;

  float horizontal_ratio = (float)engine.GetRenderer().GetScreenWidth() / 256.0f;
  float vertical_ratio  = (float)engine.GetRenderer().GetScreenHeight() / 256.0f;
  Vector2 scale = engine.ToScale(256, 256);
  int num_horizontal_tiles = (int)(horizontal_ratio + 0.5f);
  int num_vertical_tiles = (int)(vertical_ratio + 0.5f) + 1;
  for (int y = 0; y < num_vertical_tiles; ++y) {
    for (int x = 0; x < num_horizontal_tiles; ++x) {
      auto iq = std::make_unique<engine::ImageQuad>();
      if (!iq->Create(image)) {
        LOG << "Failed to create the backgroud.";
        return false;
      }
      iq->Scale(scale);
      float fx = (-1.0f + scale.x / 2) + x * scale.x;
      float fy = (1.0f + scale.y / 2) - y * scale.y;
      iq->Translate(Vector2(fx, fy));
      engine.AddDrawable(iq.get());
      bg_tiles_.push_back(std::move(iq));
    }
  }

  return true;
}

void Background::Update(float delta_time) {
  seconds_accumulated_ += delta_time;

  engine::Engine& engine = engine::Engine::Get();

  float horizontal_ratio = (float)engine.GetRenderer().GetScreenWidth() / 256.0f;
  float vertical_ratio  = (float)engine.GetRenderer().GetScreenHeight() / 256.0f;
  Vector2 scale = engine.ToScale(256, 256);
  float scroll_offset_y = fmod(-seconds_accumulated_ * 0.15f, scale.y);
  int num_horizontal_tiles = (int)(horizontal_ratio + 0.5f);
  int num_vertical_tiles = (int)(vertical_ratio + 0.5f) + 1;
  for (int y = 0; y < num_vertical_tiles; ++y) {
    for (int x = 0; x < num_horizontal_tiles; ++x) {
      float fx = (-1.0f + scale.x / 2) + x * scale.x;
      float fy = (1.0f + scale.y / 2) - y * scale.y;
      bg_tiles_[y * num_horizontal_tiles + x]->Translate(Vector2(fx, fy + scroll_offset_y));
    }
  }
}
