#include "sky_quad.h"
#include "../base/log.h"
#include "../base/random.h"
#include "../engine/engine.h"
#include <cassert>

bool SkyQuad::Create() {
  const char* vertex_description = "p2f";
  if (!shader_.Create("shaders/sky",
                                   vertex_description)) {
    LOG << "Could not create sky shader.";
    return false;
  }

  // This creates a normalized unit sized quad.
  static const float vertices[] = {
    -0.5f, -0.5f,
     0.5f, -0.5f,
    -0.5f,  0.5f,
     0.5f,  0.5f
  };
  quad_.Create(GL_TRIANGLE_STRIP, vertex_description, 4, vertices);

  scale_ = eng::Engine::Get().GetScreenSize();
  nebula_color_ = {0.962f, 0.308f, 0.112f};

  return true;
}

void SkyQuad::ContextLost() {
  quad_.Invalidate();
  shader_.Invalidate();
}

void SkyQuad::Draw() {
  sky_offset_ += Vector2(0, -0.0006f);

  shader_.Activate();
  shader_.SetUniform("scale", scale_);
  shader_.SetUniform("projection", eng::Engine::Get().GetProjectionMarix());
  shader_.SetUniform("sky_offset", sky_offset_);
  shader_.SetUniform("nebula_color", nebula_color_);

  quad_.Draw();
}
