#include "boss.h"

#include "../base/interpolation.h"
#include "../engine/engine.h"
#include "../engine/image.h"
#include "../engine/renderer/texture.h"
#include "demo.h"
#include "enemy.h"

using namespace base;
using namespace eng;

Boss::Boss()
    : boss_tex_(Engine::Get().CreateRenderResource<Texture>()) {}

Boss::~Boss() = default;

bool Boss::Initialize() {
  if (!CreateRenderResources())
    return false;

  sprite_.Create(boss_tex_, {4, 3});
  sprite_.AutoScale();
  sprite_.SetOffset((Engine::Get().GetScreenSize() + sprite_.GetScale()) *
                   Vector2(0, 0.5f));
  sprite_.SetVisible(true);

  sprite_animator_.SetMovement({0, sprite_.GetScale().y * -0.99f}, 2,
      std::bind(Acceleration, std::placeholders::_1, -1));
  sprite_animator_.Play(Animator::kMovement, false);

  sprite_animator_.SetFrames(8, 12);
  sprite_animator_.Attach(&sprite_);
  sprite_animator_.Play(Animator::kFrames, true);

  Engine& engine = Engine::Get();
  Demo* game = static_cast<Demo*>(engine.GetGame());

  game->GetEnemy().SpawnBoss(sprite_.GetOffset() -
                                sprite_.GetScale() * Vector2(0, 0.3f),
                             sprite_.GetScale() * 0.3f);

  return true;
}

void Boss::ContextLost() {
  CreateRenderResources();
}

void Boss::Update(float delta_time) {
  sprite_animator_.Update(delta_time);
}

void Boss::Draw(float frame_frac) {
  sprite_.Draw();
}

void Boss::Hit(DamageType damage_type) {
}

bool Boss::CreateRenderResources() {
  Engine& engine = Engine::Get();

  auto boss_image = engine.GetAsset<Image>("Boss_ok.png");
  if (!boss_image)
    return false;

  boss_tex_->Update(boss_image);

  return true;
}
