#include "movie_quad.h"
#include "../base/log.h"
#include "../engine/asset_manager/image.h"
#include "../base/font.h"
#include "engine.h"
#include "renderer/geometry.h"
#include "renderer/shader.h"

namespace engine {

bool MovieQuad::Create(std::vector<std::shared_ptr<const Image>> images) {
  if (images.empty())
    return false;
  scale_ = engine::Engine::Get().ToScale(images.back()->GetWidth(),
      images.back()->GetHeight());
  for (auto& image : images) {
    // TODO: make atomic.
    textures_.emplace_back(std::make_unique<Texture>());
    if (!textures_.back()->Create(image))
      return false;
  }
  return true;
}

void MovieQuad::Draw() {
  textures_[active_texture_]->Activate();

  Geometry& quad = Engine::Get().GetQuad();
  Shader& shader = Engine::Get().GetPassThroughShader();

  shader.Activate();
  shader.SetUniform("offset", offset_);
  shader.SetUniform("scale", scale_);
  shader.SetUniform("tileColor", Vector3(1, 1, 1));
  shader.SetUniform("tileImage", 0);

  quad.Draw();
}

}  // namespace engine
