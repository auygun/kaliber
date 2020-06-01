#include "texture.h"

#include <cassert>

#include "../../engine/image.h"
#include "../engine.h"
#include "render_command.h"

namespace eng {

int Texture::last_id = 0;

Texture::~Texture() {
  Destroy();
}

void Texture::Update(std::shared_ptr<const Image> image) {
  assert(image->IsImmutable());

  if (resource_id_ == 0)
    resource_id_ = ++last_id;

  width_ = image->GetWidth();
  height_ = image->GetHeight();

  auto cmd = std::make_unique<CmdUpdateTexture>();
  cmd->id = resource_id_;
  cmd->image = image;
  Engine::Get().EnqueueRenderCommand(std::move(cmd));
}

void Texture::Destroy() {
  if (resource_id_) {
    auto cmd = std::make_unique<CmdDestoryTexture>();
    cmd->id = resource_id_;
    Engine::Get().EnqueueRenderCommand(std::move(cmd));
    resource_id_ = 0;
  }
}

void Texture::Activate() {
  if (resource_id_) {
    auto cmd = std::make_unique<CmdActivateTexture>();
    cmd->id = resource_id_;
    Engine::Get().EnqueueRenderCommand(std::move(cmd));
  }
}

}  // namespace eng
