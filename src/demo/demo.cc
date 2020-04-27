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

  std::vector<std::shared_ptr<const engine::Image>> enemy_frames;
  enemy_frames.push_back(engine.GetAssetManager().GetImage("enemy1.png"));
  enemy_frames.push_back(engine.GetAssetManager().GetImage("enemy2.png"));
  enemy_frames.push_back(engine.GetAssetManager().GetImage("enemy3.png"));
  enemy_frames.push_back(engine.GetAssetManager().GetImage("enemy4.png"));
  enemy_frames.push_back(engine.GetAssetManager().GetImage("enemy5.png"));
  if (!enemy_.Create(enemy_frames)) {
    LOG << "Failed to create the sprite.";
    return false;
  }
  enemy_.SetActiveTexture(0);
  enemy_.Translate(Vector2(0.5f, 0.5f));
  engine::Engine::Get().AddDrawable(&enemy_);

  return true;
}

void Demo::Update(float delta_time) {
  seconds_accumulated_ += delta_time;
  bg_.Update(delta_time);
  if (seconds_accumulated_ > 0.1f) {
    ++next_;
    if (next_ >= enemy_.GetNumTextures())
      next_ = 0;
    enemy_.SetActiveTexture(next_);
    seconds_accumulated_ = 0;
  }
}
