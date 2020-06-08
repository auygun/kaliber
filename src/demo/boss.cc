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

  sprite_animator_.SetEndCallback(Animator::kMovement, [&]() -> void {
    Vector2 pos = {0 ,0}, scale = {1, 1};
    GetHitBox(pos, scale);
    Demo* game = static_cast<Demo*>(Engine::Get().GetGame());
    game->GetEnemy().SpawnBoss(pos, scale);
  });
  sprite_animator_.SetMovement({0, sprite_.GetScale().y * -0.99f}, 2,
      std::bind(Acceleration, std::placeholders::_1, -1));
  sprite_animator_.SetFrames(8, 12);
  sprite_animator_.Attach(&sprite_);

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

void Boss::Start() {
  sprite_.SetVisible(true);
  sprite_.SetOffset((Engine::Get().GetScreenSize() + sprite_.GetScale()) *
                   Vector2(0, 0.5f));
  sprite_animator_.Play(Animator::kMovement | Animator::kFrames, false);
}

void Boss::Hit(DamageType damage_type) {
}

void Boss::GetHitBox(Vector2& pos, Vector2& scale) {
  pos = sprite_.GetOffset() - sprite_.GetScale() * Vector2(0, 0.3f);
  scale = sprite_.GetScale() * 0.3f;
}

bool Boss::CreateRenderResources() {
  Engine& engine = Engine::Get();

  auto boss_image = engine.GetAsset<Image>("Boss_ok.png");
  if (!boss_image)
    return false;

  boss_tex_->Update(boss_image);

  return true;
}
