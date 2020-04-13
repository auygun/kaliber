#include "solid_quad.h"

#include <memory>

#include "engine.h"
#include "renderer/geometry.h"
#include "renderer/shader.h"

namespace eng {

void SolidQuad::Draw() {
  if (!IsVisible())
    return;

  std::shared_ptr<Geometry> quad = Engine::Get().GetQuad();
  std::shared_ptr<Shader> shader = Engine::Get().GetSolidShader();

  shader->Activate();
  shader->SetUniform("offset", offset_);
  shader->SetUniform("scale", scale_);
  shader->SetUniform("pivot", pivot_);
  shader->SetUniform("rotation", rotation_);
  shader->SetUniform("projection", Engine::Get().GetProjectionMarix());
  shader->SetUniform("color", color_);

  quad->Draw();
}

}  // namespace eng
