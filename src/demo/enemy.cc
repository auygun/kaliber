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
  sprite_.SetOffset(Vector2(0.5f, 0.5f));
  engine::Engine::Get().AddDrawable(&sprite_);

  frame_animator_.AttachDrawable(&sprite_);
  frame_animator_.Play();
  return true;
}

void Enemy::Update(float delta_time) {
  frame_animator_.Update(delta_time);
}
