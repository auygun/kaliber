#include "solid_quad.h"

#include <memory>

#include "../base/log.h"
#include "engine.h"
#include "renderer/geometry.h"
#include "renderer/shader.h"

namespace eng {

void SolidQuad::Draw(float frame_frac) {
  DCHECK(IsVisible());

  Shader* shader = Engine::Get().GetSolidShader();

  shader->Activate();
  shader->SetUniform("offset", offset_);
  shader->SetUniform("scale", scale_);
  shader->SetUniform("pivot", pivot_);
  shader->SetUniform("rotation", rotation_);
  shader->SetUniform("projection", Engine::Get().GetProjectionMarix());
  shader->SetUniform("color", color_);

  Engine::Get().GetQuad()->Draw();
}

}  // namespace eng
