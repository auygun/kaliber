#include "render_resource.h"

#include <cassert>

#include "renderer.h"

namespace eng {

RenderResource::RenderResource(unsigned resource_id,
                               std::shared_ptr<void> impl_data,
                               Renderer* renderer)
    : resource_id_(resource_id), impl_data_(impl_data), renderer_(renderer) {}

RenderResource::~RenderResource() {
  renderer_->ReleaseResource(resource_id_);
}

}  // namespace eng
