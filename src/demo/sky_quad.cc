#include "sky_quad.h"
#include "../base/log.h"
#include "../base/random.h"
#include "../engine/engine.h"
#include <cassert>

namespace engine {

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
  if (!quad_.Create(GL_TRIANGLE_STRIP, vertex_description, 4, vertices)) {
    LOG << "Could not create quad geometry.";
    return false;
  }

  scale_ = Engine::Get().GetScreenSize();

  nebula_color_ = {RandomFloat(-0.8f, 0.2f),
                   RandomFloat(-0.1f, 0.4f),
                   RandomFloat(-0.1f, 0.4f)};

  return true;
}

void SkyQuad::Draw() {
  sky_offset_ += Vector2(0, -0.0006f);

  shader_.Activate();
  shader_.SetUniform("scale", scale_);
  shader_.SetUniform("projection", Engine::Get().GetRenderer().projection());
  shader_.SetUniform("resolution", Vector2(2000, 2000));
  shader_.SetUniform("sky_offset", sky_offset_);
  shader_.SetUniform("nebula_color", nebula_color_);

  quad_.Draw();
}

}  // namespace engine
