#include "texture.h"

#include "../../base/log.h"
#include "../image.h"
#include "renderer.h"

namespace eng {

Texture::Texture(Renderer* renderer) : RenderResource(renderer) {}

Texture::~Texture() {
  Destroy();
}

void Texture::Update(std::unique_ptr<Image> image) {
  if (!IsValid())
    resource_id_ = renderer_->CreateTexture();
  width_ = image->GetWidth();
  height_ = image->GetHeight();
  renderer_->UpdateTexture(resource_id_, std::move(image));
}

void Texture::Destroy() {
  if (IsValid()) {
    renderer_->DestroyTexture(resource_id_);
    resource_id_ = 0;
    DLOG << "Texture destroyed. resource_id: " << resource_id_;
  }
}

void Texture::Activate() {
  if (IsValid())
    renderer_->ActivateTexture(resource_id_);
}

}  // namespace eng
