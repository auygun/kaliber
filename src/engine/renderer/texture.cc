#include "texture.h"

#include "../../base/log.h"
#include "../image.h"
#include "render_command.h"
#include "renderer.h"

namespace eng {

Texture::Texture(unsigned resource_id,
                 std::shared_ptr<void> impl_data,
                 Renderer* renderer)
    : RenderResource(resource_id, impl_data, renderer) {}

Texture::~Texture() {
  Destroy();
}

void Texture::Update(std::unique_ptr<Image> image) {
  valid_ = true;
  width_ = image->GetWidth();
  height_ = image->GetHeight();

  auto cmd = std::make_unique<CmdUpdateTexture>();
  cmd->image = std::move(image);
  cmd->impl_data = impl_data_;
  renderer_->EnqueueCommand(std::move(cmd));
}

void Texture::Destroy() {
  if (valid_) {
    auto cmd = std::make_unique<CmdDestoryTexture>();
    cmd->impl_data = impl_data_;
    renderer_->EnqueueCommand(std::move(cmd));
    valid_ = false;
    DLOG << "Texture destroyed. resource_id: " << resource_id_;
  }
}

void Texture::Activate() {
  if (valid_) {
    auto cmd = std::make_unique<CmdActivateTexture>();
    cmd->impl_data = impl_data_;
    renderer_->EnqueueCommand(std::move(cmd));
  }
}

}  // namespace eng
