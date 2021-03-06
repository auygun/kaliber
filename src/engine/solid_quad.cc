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
  shader->SetUniform("offset", position_);
  shader->SetUniform("scale", GetSize());
  shader->SetUniform("rotation", rotation_);
  shader->SetUniform("projection", Engine::Get().GetProjectionMatrix());
  shader->SetUniform("color", color_);
  shader->UploadUniforms();

  Engine::Get().GetQuad()->Draw();
}

}  // namespace eng
