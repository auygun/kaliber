#include "texture.h"

#include <cassert>

#include "../../engine/image.h"
#include "../engine.h"
#include "render_command.h"

namespace eng {

Texture::~Texture() {
  Destroy();
}

void Texture::Create(std::shared_ptr<const Image> image) {
  assert(image->IsImmutable());

  Destroy();
  resource_id_ = Engine::Get().AcquireTextureResource(image);
}

void Texture::Update(std::shared_ptr<const Image> image) {
  assert(image->IsImmutable());

  if (resource_id_) {
    auto cmd = std::make_unique<CmdUpdateTexture>();
    cmd->id = resource_id_;
    cmd->image = image;
    Engine::Get().EnqueueRenderCommand(std::move(cmd));
  } else {
    Create(image);
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
