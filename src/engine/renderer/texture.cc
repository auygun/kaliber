#include "../../engine/asset_manager/image.h"
#include "../../base/log.h"
#include "texture.h"
#include "render_command.h"
#include "../engine.h"

namespace engine {

Texture::~Texture() {
  Destroy();
}

// TODO: separate create and update.
bool Texture::Create(std::shared_ptr<const Image> image) {
  Destroy();
  id = Engine::Get().GetRenderer().AcquireTextureResource(image);
  return id > 0;
}

void Texture::Destroy() {
  if (id)
    Engine::Get().GetRenderer().ReturnTextureResource(id);
}

void Texture::Activate() {
  if (id) {
    auto cmd = std::make_unique<CmdActivateTexture>();
    cmd->id = id;
    Engine::Get().GetRenderer().EnqueueCommand(std::move(cmd));
  }
}

} // namespace engine
