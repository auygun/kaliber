#include "geometry.h"

#include "../mesh.h"
#include "renderer.h"

namespace eng {

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

}  // namespace eng
