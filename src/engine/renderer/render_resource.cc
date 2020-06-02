#include "render_resource.h"

#include <cassert>

#include "renderer.h"

namespace eng {

RenderResource::RenderResource(unsigned resource_id, Renderer* renderer)
    : resource_id_(resource_id), renderer_(renderer) {}

RenderResource::~RenderResource() {
  renderer_->ReleaseResource(resource_id_);
}

}  // namespace eng
