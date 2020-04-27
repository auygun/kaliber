#include "../../engine/asset_manager/image.h"
#include "../../base/log.h"
#include "texture.h"
#include "render_command.h"
#include "../engine.h"

namespace engine {

int Texture::last_id = 0;

Texture::~Texture() {
  Destroy();
}

// TODO: separate create and update.
bool Texture::Create(std::shared_ptr<const Image> image) {
  if (id == 0 && !image->GetName().empty()) {
    id = Engine::Get().GetRenderer().GetTexture(image->GetName());
    if (id > 0)
      return true;
  }
  if (id == 0) {
    id = ++last_id;
    Engine::Get().GetRenderer().AddTexture(image->GetName(), id);
  }

  auto cmd = std::make_unique<CmdCreateTexture>();
  cmd->id = id;
  cmd->image = image;
  Engine::Get().GetRenderer().EnqueueCommand(std::move(cmd));
  return true;
}

void Texture::Destroy() {
  // TODO: This is wrong. Same texture id is used by many texture instances.
  // Create a resource manager.
  if (id) {
    auto cmd = std::make_unique<CmdDestoryTexture>();
    cmd->id = id;
    Engine::Get().GetRenderer().EnqueueCommand(std::move(cmd));
    id = 0;
  }
}

void Texture::Activate() {
  if (id) {
    auto cmd = std::make_unique<CmdActivateTexture>();
    cmd->id = id;
    Engine::Get().GetRenderer().EnqueueCommand(std::move(cmd));
  }
}

} // namespace engine
