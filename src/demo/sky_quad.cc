#include "sky_quad.h"

#include "../base/log.h"
#include "../base/misc.h"
#include "../engine/engine.h"
#include "../engine/shader_source.h"

using namespace base;
using namespace eng;

bool SkyQuad::Create() {
  eng::Engine& engine = eng::Engine::Get();

  auto sky_source = engine.GetShaderAsset("sky");
  if (!sky_source)
    return false;
  shader_.Create(sky_source, engine.GetQuad().vertex_description());

  scale_ = engine.GetScreenSize();

  color_animator_.Attach(this);

  return true;
}

void SkyQuad::Update(float delta_time) {
  sky_offset_ += {0, delta_time * -0.04f};
  color_animator_.Update(delta_time);
}

void SkyQuad::Draw(float frame_frac) {
  Vector2 sky_offset = Lerp(last_sky_offset_, sky_offset_, frame_frac);

  shader_.Activate();
  shader_.SetUniform("scale", scale_);
  shader_.SetUniform("projection", eng::Engine::Get().GetProjectionMarix());
  shader_.SetUniform("sky_offset", sky_offset);
  shader_.SetUniform("nebula_color",
      {nebula_color_.x, nebula_color_.y, nebula_color_.z});

  eng::Engine::Get().GetQuad().Draw();
  last_sky_offset_ = sky_offset_;
}

void SkyQuad::ContextLost() {
  shader_.Invalidate();
}

void SkyQuad::SwitchColor(const Vector4& color) {
  color_animator_.SetBlending(color, 5);
  color_animator_.Play(Animator::kBlending, false);
}
