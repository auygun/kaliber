#include "movie_quad.h"
#include "../base/log.h"
#include "../engine/asset_manager/image.h"
#include "../base/font.h"
#include "engine.h"
#include "renderer/geometry.h"
#include "renderer/shader.h"
#include <cassert>

namespace engine {

bool MovieQuad::Create(std::vector<std::shared_ptr<const Image>> images) {
  if (images.empty())
    return false;
  SetScale(engine::Engine::Get().ToScale(images.back()->GetWidth(),
      images.back()->GetHeight()));
  for (auto& image : images) {
    // TODO: make atomic.
    textures_.emplace_back(std::make_unique<Texture>());
    if (!textures_.back()->Create(image))
      return false;
  }
  return true;
}

size_t MovieQuad::GetNumFrames() {
  return textures_.size();
}

size_t MovieQuad::GetCurrentFrame() {
  return active_texture_;
}

void MovieQuad::SetCurrentFrame(size_t frame) {
  assert(frame < textures_.size());
  active_texture_ = frame;
}

void MovieQuad::Draw() {
  textures_[active_texture_]->Activate();

  Geometry& quad = Engine::Get().GetQuad();
  Shader& shader = Engine::Get().GetPassThroughShader();

  shader.Activate();
  shader.SetUniform("offset", offset());
  shader.SetUniform("scale", scale());
  shader.SetUniform("tileColor", Vector3(1, 1, 1));
  shader.SetUniform("tileImage", 0);

  quad.Draw();
}

}  // namespace engine
