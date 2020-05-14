#include "solid_quad.h"
#include "renderer/geometry.h"
#include "renderer/shader.h"
#include "engine.h"

namespace eng {

void SolidQuad::Draw() {
  Geometry& quad = Engine::Get().GetQuad();
  Shader& shader = Engine::Get().GetSolidShader();

  shader.Activate();
  shader.SetUniform("offset", offset_);
  shader.SetUniform("scale", scale_);
  shader.SetUniform("pivot", pivot_);
  shader.SetUniform("rotation", rotation_);
  shader.SetUniform("projection", Engine::Get().GetProjectionMarix());
  shader.SetUniform("color", color_);

  quad.Draw();
}

}  // namespace eng
