#include "texture.h"

#include <cassert>

#include "../../engine/image.h"
#include "../engine.h"
#include "render_command.h"

namespace eng {

Texture::~Texture() {
  Destroy();
}

bool Texture::Create(const std::string &name) {
  Destroy();
  resource_id_ = Engine::Get().GetTextureResource(name, width_, height_);
  return resource_id_ > 0;
}

void Texture::Update(std::shared_ptr<const Image> image) {
  assert(image->IsImmutable());

  width_ = image->GetWidth();
  height_ = image->GetHeight();

  if (resource_id_) {
    auto cmd = std::make_unique<CmdUpdateTexture>();
    cmd->id = resource_id_;
    cmd->image = image;
    Engine::Get().EnqueueRenderCommand(std::move(cmd));
  } else {
    Destroy();
    resource_id_ = Engine::Get().AcquireTextureResource(image);
  }
}

void Texture::Destroy() {
  if (resource_id_) {
    Engine::Get().ReturnTextureResource(resource_id_);
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
