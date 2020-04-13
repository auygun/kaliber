#include "geometry.h"

#include <cassert>

#include "../engine.h"
#include "../mesh.h"
#include "render_command.h"
#include "renderer.h"

namespace eng {

Geometry::Geometry(unsigned resource_id,
                   std::shared_ptr<void> impl_data,
                   Renderer* renderer)
    : RenderResource(resource_id, impl_data, renderer) {}

Geometry::~Geometry() {
  Destroy();
}

void Geometry::Create(std::shared_ptr<const Mesh> mesh) {
  assert(mesh->IsImmutable());

  Destroy();
  valid_ = true;

  vertex_description_ = mesh->vertex_description();

  auto cmd = std::make_unique<CmdCreateGeometry>();
  cmd->mesh = mesh;
  cmd->impl_data = std::static_pointer_cast<void>(impl_data_);
  renderer_->EnqueueCommand(std::move(cmd));
}

void Geometry::Destroy() {
  if (valid_) {
    auto cmd = std::make_unique<CmdDestroyGeometry>();
    cmd->impl_data = std::static_pointer_cast<void>(impl_data_);
    renderer_->EnqueueCommand(std::move(cmd));
    valid_ = false;
  }
}

void Geometry::Draw() {
  if (valid_) {
    auto cmd = std::make_unique<CmdDrawGeometry>();
    cmd->impl_data = std::static_pointer_cast<void>(impl_data_);
    renderer_->EnqueueCommand(std::move(cmd));
  }
}

}  // namespace eng
