#include "texture.h"

#include "../../base/log.h"
#include "../image.h"
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
  renderer_->UpdateTexture(impl_data_, std::move(image));
}

void Texture::Destroy() {
  if (valid_) {
    renderer_->DestroyTexture(impl_data_);
    valid_ = false;
    DLOG << "Texture destroyed. resource_id: " << resource_id_;
  }
}

void Texture::Activate() {
  if (valid_)
    renderer_->ActivateTexture(impl_data_);
}

}  // namespace eng
