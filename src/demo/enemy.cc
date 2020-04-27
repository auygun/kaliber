#include "enemy.h"
#include "../base/log.h"
#include "../engine/engine.h"
#include <memory>

bool Enemy::Initialize() {
  engine::Engine& engine = engine::Engine::Get();

  std::vector<std::shared_ptr<const engine::Image>> enemy_frames;
  enemy_frames.push_back(engine.GetAssetManager().GetImage("enemy1.png"));
  enemy_frames.push_back(engine.GetAssetManager().GetImage("enemy2.png"));
  enemy_frames.push_back(engine.GetAssetManager().GetImage("enemy3.png"));
  enemy_frames.push_back(engine.GetAssetManager().GetImage("enemy4.png"));
  enemy_frames.push_back(engine.GetAssetManager().GetImage("enemy5.png"));
  if (!sprite_.Create(enemy_frames)) {
    LOG << "Failed to create the sprite.";
    return false;
  }
  sprite_.SetActiveTexture(0);
  sprite_.Translate(Vector2(0.5f, 0.5f));
  engine::Engine::Get().AddDrawable(&sprite_);
  return true;
}

void Enemy::Update(float delta_time) {
  seconds_accumulated_ += delta_time;
  if (seconds_accumulated_ > 0.1f) {
    seconds_accumulated_ = 0;
    size_t next = sprite_.active_texture() + 1;
    if (sprite_.active_texture() + 1 >= sprite_.GetNumTextures())
      sprite_.SetActiveTexture(0);
    else
      sprite_.SetActiveTexture(next);
  }
}
