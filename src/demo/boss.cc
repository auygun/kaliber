#include "boss.h"

#include "../engine/engine.h"
#include "../engine/image.h"
#include "../engine/renderer/texture.h"

using namespace base;
using namespace eng;

Boss::Boss()
    : boss_tex_(Engine::Get().CreateRenderResource<Texture>()) {}

Boss::~Boss() = default;

bool Boss::Initialize() {
  if (!CreateRenderResources())
    return false;

  sprite.Create(boss_tex_, {4, 3});
  sprite.AutoScale();
  sprite.SetVisible(true);

  return true;
}

void Boss::ContextLost() {
  CreateRenderResources();
}

void Boss::Update(float delta_time) {
}

void Boss::Draw(float frame_frac) {
  sprite.Draw();
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
