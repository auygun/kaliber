#include "image_quad.h"
#include "../base/log.h"
#include "../base/font.h"
#include "engine.h"
#include "asset_manager/image.h"
#include "renderer/geometry.h"
#include "renderer/shader.h"

namespace engine {

bool ImageQuad::Create(std::shared_ptr<const Image> image) {
  SetScale(engine::Engine::Get().ToScale(image->GetWidth(),
      image->GetHeight()));
  return texture_.Create(image);
}

void ImageQuad::Draw() {
  texture_.Activate();

  Geometry& quad = Engine::Get().GetQuad();
  Shader& shader = Engine::Get().GetPassThroughShader();

  shader.Activate();
  shader.SetUniform("offset", offset());
  shader.SetUniform("scale", scale());
  shader.SetUniform("rotation", rotation());
  shader.SetUniform("projection", engine::Engine::Get().GetRenderer().projection());
  shader.SetUniform("tile_color", Vector3(1, 1, 1));
  shader.SetUniform("tile_image", 0);

  quad.Draw();
}

}  // namespace engine
