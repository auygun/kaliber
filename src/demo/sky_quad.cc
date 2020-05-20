#include "sky_quad.h"

#include "../base/log.h"
#include "../base/misc.h"
#include "../engine/engine.h"
#include "../engine/shader_source.h"

using base::Vector2;

bool SkyQuad::Create() {
  eng::Engine& engine = eng::Engine::Get();

  auto sky_source = engine.GetShaderAsset("sky");
  if (!sky_source)
    return false;
  shader_.Create(sky_source, engine.GetQuad().vertex_description());

  scale_ = engine.GetScreenSize();
  nebula_color_ = {0.962f, 0.308f, 0.112f};

  return true;
}

void SkyQuad::Draw(float frame_frac) {
  Vector2 sky_offset = Lerp(last_sky_offset_, sky_offset_, frame_frac);

  shader_.Activate();
  shader_.SetUniform("scale", scale_);
  shader_.SetUniform("projection", eng::Engine::Get().GetProjectionMarix());
  shader_.SetUniform("sky_offset", sky_offset);
  shader_.SetUniform("nebula_color", nebula_color_);

  eng::Engine::Get().GetQuad().Draw();
  last_sky_offset_ = sky_offset_;
}

void SkyQuad::ContextLost() {
  shader_.Invalidate();
}
