#include "../../base/image.h"
#include "../../base/log.h"
#include "texture.h"
#include "render_command.h"
#include "../engine.h"

namespace engine {

int Texture::last_id = 0;

Texture::~Texture() {
  Destroy();
}

bool Texture::Create(std::unique_ptr<Image> image) {
  Destroy();

  auto cmd = std::make_unique<CmdCreateTexture>();
  id = ++last_id;
  cmd->id = id;
  cmd->image = std::move(image);
  Engine::Get().GetRenderer().EnqueueCommand(std::move(cmd));

  return true;
}

void Texture::Destroy() {
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
