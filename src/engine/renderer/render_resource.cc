#include "render_resource.h"

#include <cassert>

#include "../engine.h"

namespace eng {

RenderResource::RenderResource(unsigned resource_id)
    : resource_id_(resource_id) {}

RenderResource::~RenderResource() {
  Engine::Get().ReleaseResource(resource_id_);
}

}  // namespace eng
