#include "sky_quad.h"
#include "../base/log.h"
#include "../base/random.h"
#include "../engine/engine.h"
#include "../engine/shader_source.h"
#include "../engine/renderer/geometry.h"
#include <cassert>

bool SkyQuad::Create() {
  eng::Engine& engine = eng::Engine::Get();

  auto sky_code = engine.GetShaderAsset("sky");
  if (!sky_code)
    return false;
  shader_.Create(sky_code, engine.GetVertexDescription());

  scale_ = engine.GetScreenSize();
  nebula_color_ = {0.962f, 0.308f, 0.112f};

  return true;
}

void SkyQuad::Draw() {
  shader_.Activate();
  shader_.SetUniform("scale", scale_);
  shader_.SetUniform("projection", eng::Engine::Get().GetProjectionMarix());
  shader_.SetUniform("sky_offset", sky_offset_);
  shader_.SetUniform("nebula_color", nebula_color_);

  eng::Engine::Get().GetQuad().Draw();
}

void SkyQuad::ContextLost() {
  shader_.Invalidate();
}
