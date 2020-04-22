#include "../../base/log.h"
#include "../renderer/renderer.h"
// #include "../engine.h"
#include "igeometry.h"
#include "render_command.h"
#include "../engine.h"

namespace engine {

int IGeometry::last_id = 0;

IGeometry::~IGeometry() {
  Destroy();
}

bool IGeometry::Create(unsigned int primitive,
                       const std::string& vertex_description,
                       int num_vertices,
                       const void* vertices,
                       unsigned int index_description,
                       int num_indices,
                       const void* indices) {
  Destroy();

  auto cmd = std::make_unique<CmdCreateGeometry>();
  id = ++last_id;
  cmd->id = id;
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

void IGeometry::Destroy() {
  if (id) {
    auto cmd = std::make_unique<CmdDestroyGeometry>();
    cmd->id = id;
    Engine::Get().GetRenderer().EnqueueCommand(std::move(cmd));
    id = 0;
  }
}

void IGeometry::Draw() {
  if (id) {
    auto cmd = std::make_unique<CmdDrawGeometry>();
    cmd->id = id;
    Engine::Get().GetRenderer().EnqueueCommand(std::move(cmd));
  }
}

} // namespace engine
