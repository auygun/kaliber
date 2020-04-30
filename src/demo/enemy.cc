#include "enemy.h"
#include "../base/log.h"
#include "../engine/asset_manager/image.h"
#include "../engine/engine.h"
#include <memory>

bool Enemy::Initialize() {
  engine::Engine& engine = engine::Engine::Get();

  std::shared_ptr<const engine::Image> enemy_frames =
      engine.GetAssetManager().GetImage("enemy_anims_01_frames_ok.png");
  if (!sprite_.Create(enemy_frames, {10, 6})) {
    LOG << "Failed to create the sprite.";
    return false;
  }
  sprite_.SetVisible(true);
  sprite_.SetOffset(Vector2(0.5f, 0.5f));
  engine::Engine::Get().AddDrawable(&sprite_);

  frame_animator_.AttachFrameController(&sprite_);
  frame_animator_.Play();
  return true;
}

void Enemy::Update(float delta_time) {
  sprite_.Rotate(engine::Engine::Get().seconds_accumulated());
  frame_animator_.Update(delta_time);
}
