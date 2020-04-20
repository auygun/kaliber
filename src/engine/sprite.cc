#include "sprite.h"
#include "../base/log.h"
#include "engine.h"
#include "renderer/geometry.h"
#include "renderer/shader.h"

namespace engine {

bool Sprite::Create(const std::string& asset_name,
                    const Vector2& offset,
                    const Vector2& scale) {
  if (!image_.Load(asset_name.c_str()))
    return false;

  offset_ = offset;
  scale_ = scale;

  return texture_.Create(image_);
}

void Sprite::Draw(const Vector2& offset) {
  Vector2 draw_offset = offset_ + offset;

  texture_.Activate();

  Geometry& quad = Engine::Get().GetQuad();
  Shader& shader = Engine::Get().GetPassThroughShader();

  shader.Activate();
  shader.SetUniform("offset", draw_offset);
  shader.SetUniform("scale", scale_);
  shader.SetUniform("tileColor", Vector3(1, 1, 1));
  shader.SetUniform("tileImage", 0);

  quad.Draw();
}

}  // namespace engine
