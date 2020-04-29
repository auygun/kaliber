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

  Vector2 screen_size = engine.GetScreenSize();
  Vector2 scale = engine.ToScale(
      Vector2(image->GetWidth(), image->GetHeight()));

  Vector2 num_tiles = screen_size / scale;
  int num_horizontal_tiles = (int)(num_tiles.x + 0.5f);
  int num_vertical_tiles = (int)(num_tiles.y + 0.5f) + 1;

  for (int y = 0; y < num_vertical_tiles; ++y) {
    for (int x = 0; x < num_horizontal_tiles; ++x) {
      auto iq = std::make_unique<engine::ImageQuad>();
      if (!iq->Create(image)) {
        LOG << "Failed to create the backgroud.";
        return false;
      }
      iq->SetScale(scale);
      float fx = (-screen_size.x / 2 + scale.x / 2) + x * scale.x;
      float fy = (screen_size.y / 2 + scale.y / 2) - y * scale.y;
      iq->SetOffset(Vector2(fx, fy));
      engine.AddDrawable(iq.get());
      draw_animator_.AttachDrawable(iq.get());
      bg_tiles_.push_back(std::move(iq));
    }
  }

  draw_animator_.SetMovement(Vector2(0, -1), scale.y);
  draw_animator_.Play();

  return true;
}

void Background::Update(float delta_time) {
  draw_animator_.Update(delta_time);
}
