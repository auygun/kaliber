#include "texture.h"

#include <cassert>

#include "../../engine/image.h"
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

void Texture::Update(std::shared_ptr<const Image> image) {
  assert(image->IsImmutable());

  valid_ = true;
  width_ = image->GetWidth();
  height_ = image->GetHeight();

  auto cmd = std::make_unique<CmdUpdateTexture>();
  cmd->image = image;
  cmd->impl_data = std::static_pointer_cast<void>(impl_data_);
  renderer_->EnqueueCommand(std::move(cmd));
}

void Texture::Destroy() {
  if (valid_) {
    auto cmd = std::make_unique<CmdDestoryTexture>();
    cmd->impl_data = std::static_pointer_cast<void>(impl_data_);
    renderer_->EnqueueCommand(std::move(cmd));
    valid_ = false;
  }
}

void Texture::Activate() {
  if (valid_) {
    auto cmd = std::make_unique<CmdActivateTexture>();
    cmd->impl_data = std::static_pointer_cast<void>(impl_data_);
    renderer_->EnqueueCommand(std::move(cmd));
  }
}

}  // namespace eng
