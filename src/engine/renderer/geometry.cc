#include "../../base/log.h"
#include "../renderer/renderer.h"
// #include "../engine.h"
#include "geometry.h"
#include "render_command.h"
#include "../engine.h"

namespace engine {

int Geometry::last_id = 0;

Geometry::~Geometry() {
  Destroy();
}

bool Geometry::Create(unsigned int primitive,
                       const std::string& vertex_description,
                       int num_vertices,
                       const void* vertices,
                       unsigned int index_description,
                       int num_indices,
                       const void* indices) {
  Destroy();

  auto cmd = std::make_unique<CmdCreateGeometry>();
  resource_id_ = ++last_id;
  cmd->id = resource_id_;
  cmd->primitive = primitive;
  cmd->vertex_description = vertex_description;
  cmd->num_vertices = num_vertices;
  cmd->vertices = vertices;
  cmd->index_description = index_description;
  cmd->num_indices = num_indices;
  cmd->indices = indices;
  Engine::Get().GetRenderer().EnqueueCommand(std::move(cmd));
  return true;
}

void Geometry::Destroy() {
  if (resource_id_) {
    auto cmd = std::make_unique<CmdDestroyGeometry>();
    cmd->id = resource_id_;
    Engine::Get().GetRenderer().EnqueueCommand(std::move(cmd));
    resource_id_ = 0;
  }
}

void Geometry::Draw() {
  if (resource_id_) {
    auto cmd = std::make_unique<CmdDrawGeometry>();
    cmd->id = resource_id_;
    Engine::Get().GetRenderer().EnqueueCommand(std::move(cmd));
  }
}

} // namespace engine
