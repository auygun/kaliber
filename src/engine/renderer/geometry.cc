#include "geometry.h"

#include <cassert>

#include "../engine.h"
#include "../mesh.h"
#include "render_command.h"

namespace eng {

Geometry::Geometry(unsigned resource_id) : RenderResource(resource_id) {}

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
  Engine::Get().EnqueueRenderCommand(std::move(cmd));
}

void Geometry::Destroy() {
  if (valid_) {
    auto cmd = std::make_unique<CmdDestroyGeometry>();
    cmd->impl_data = std::static_pointer_cast<void>(impl_data_);
    Engine::Get().EnqueueRenderCommand(std::move(cmd));
    valid_ = false;
  }
}

void Geometry::Draw() {
  if (valid_) {
    auto cmd = std::make_unique<CmdDrawGeometry>();
    cmd->impl_data = std::static_pointer_cast<void>(impl_data_);
    Engine::Get().EnqueueRenderCommand(std::move(cmd));
  }
}

}  // namespace eng
