#include "geometry.h"

#include "../engine.h"
#include "../mesh.h"
#include "render_command.h"

namespace eng {

int Geometry::last_id = 0;

Geometry::~Geometry() {
  Destroy();
}

void Geometry::Create(std::shared_ptr<const Mesh> mesh) {
  Destroy();

  vertex_description_ = mesh->vertex_description();

  auto cmd = std::make_unique<CmdCreateGeometry>();
  resource_id_ = ++last_id;
  cmd->id = resource_id_;
  cmd->mesh = mesh;
  cmd->impl_data = std::static_pointer_cast<void>(impl_data_);
  Engine::Get().EnqueueRenderCommand(std::move(cmd));
}

void Geometry::Destroy() {
  if (resource_id_) {
    auto cmd = std::make_unique<CmdDestroyGeometry>();
    cmd->id = resource_id_;
    cmd->impl_data = std::static_pointer_cast<void>(impl_data_);
    Engine::Get().EnqueueRenderCommand(std::move(cmd));
    resource_id_ = 0;
  }
}

void Geometry::Draw() {
  if (resource_id_) {
    auto cmd = std::make_unique<CmdDrawGeometry>();
    cmd->id = resource_id_;
    cmd->impl_data = std::static_pointer_cast<void>(impl_data_);
    Engine::Get().EnqueueRenderCommand(std::move(cmd));
  }
}

}  // namespace eng
