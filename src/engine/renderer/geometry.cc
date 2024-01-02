#include "engine/renderer/geometry.h"

#include "engine/asset/mesh.h"
#include "engine/renderer/renderer.h"

namespace eng {

Geometry::Geometry() : RenderResource(nullptr) {}

Geometry::Geometry(Renderer* renderer) : RenderResource(renderer) {}

Geometry::~Geometry() {
  Destroy();
}

void Geometry::Create(std::unique_ptr<Mesh> mesh) {
  Destroy();
  vertex_description_ = mesh->vertex_description();
  primitive_ = mesh->primitive();
  resource_id_ = renderer_->CreateGeometry(std::move(mesh));
}

void Geometry::Create(Primitive primitive,
                      VertexDescription vertex_description,
                      DataType index_description) {
  Destroy();
  vertex_description_ = vertex_description;
  primitive_ = primitive;
  resource_id_ = renderer_->CreateGeometry(primitive, vertex_description,
                                           index_description);
}

void Geometry::Update(size_t num_vertices,
                      const void* vertices,
                      size_t num_indices,
                      const void* indices) {
  if (IsValid())
    renderer_->UpdateGeometry(resource_id_, num_vertices, vertices, num_indices,
                              indices);
}

void Geometry::Destroy() {
  if (IsValid()) {
    renderer_->DestroyGeometry(resource_id_);
    resource_id_ = 0;
  }
}

void Geometry::Draw() {
  if (IsValid())
    renderer_->Draw(resource_id_);
}

void Geometry::Draw(uint64_t num_indices, uint64_t start_offset) {
  if (IsValid())
    renderer_->Draw(resource_id_, num_indices, start_offset);
}

}  // namespace eng
