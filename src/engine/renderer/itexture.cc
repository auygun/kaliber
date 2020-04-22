#include "../../base/image.h"
#include "../../base/log.h"
#include "itexture.h"
#include "render_command.h"
#include "../engine.h"

namespace engine {

int ITexture::last_id = 0;

ITexture::~ITexture() {
  Destroy();
}

bool ITexture::Create(std::unique_ptr<Image> image) {
  Destroy();

  auto cmd = std::make_unique<CmdCreateTexture>();
  id = ++last_id;
  cmd->id = id;
  cmd->image = std::move(image);
  Engine::Get().GetRenderer().EnqueueCommand(std::move(cmd));

  return true;
}

void ITexture::Destroy() {
  if (id) {
    auto cmd = std::make_unique<CmdDestoryTexture>();
    cmd->id = id;
    Engine::Get().GetRenderer().EnqueueCommand(std::move(cmd));
    id = 0;
  }
}

void ITexture::Activate() {
  if (id) {
    auto cmd = std::make_unique<CmdActivateTexture>();
    cmd->id = id;
    Engine::Get().GetRenderer().EnqueueCommand(std::move(cmd));
  }
}

} // namespace engine
