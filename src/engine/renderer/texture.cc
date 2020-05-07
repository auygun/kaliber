#include "../../engine/asset_manager/image.h"
#include "../../base/log.h"
#include "texture.h"
#include "render_command.h"
#include "../engine.h"

namespace eng {

Texture::~Texture() {
  Destroy();
}

// TODO: separate create and update.
bool Texture::Create(std::shared_ptr<const Image> image) {
  Destroy();
  resource_id_ = Engine::Get().GetRenderer().AcquireTextureResource(image);
  return resource_id_ > 0;
}

void Texture::Destroy() {
  if (resource_id_)
    Engine::Get().GetRenderer().ReturnTextureResource(resource_id_);
}

void Texture::Activate() {
  if (resource_id_) {
    auto cmd = std::make_unique<CmdActivateTexture>();
    cmd->id = resource_id_;
    Engine::Get().GetRenderer().EnqueueCommand(std::move(cmd));
  }
}

} // namespace eng
