#include "sky_quad.h"
#include "../base/log.h"
#include "../base/random.h"
#include "../engine/engine.h"
#include "../engine/renderer/geometry.h"
#include <cassert>

bool SkyQuad::Create() {
  if (!shader_.Create("shaders/sky",
                      eng::Engine::Get().GetVertexDescription())) {
    LOG << "Could not create sky shader.";
    return false;
  }

  scale_ = eng::Engine::Get().GetScreenSize();
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
