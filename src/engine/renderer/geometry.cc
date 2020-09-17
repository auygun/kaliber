#include "geometry.h"

#include "../mesh.h"
#include "renderer.h"

namespace eng {

Geometry::Geometry(unsigned resource_id,
                   std::shared_ptr<void> impl_data,
                   Renderer* renderer)
    : RenderResource(resource_id, impl_data, renderer) {}

Geometry::~Geometry() {
  Destroy();
}

void Geometry::Create(std::unique_ptr<Mesh> mesh) {
  Destroy();
  valid_ = true;
  vertex_description_ = mesh->vertex_description();
  primitive_ = mesh->primitive();
  renderer_->CreateGeometry(impl_data_, std::move(mesh));
}

void Geometry::Destroy() {
  if (valid_) {
    renderer_->DestroyGeometry(impl_data_);
    valid_ = false;
  }
}

void Geometry::Draw() {
  if (valid_)
    renderer_->Draw(impl_data_);
}

}  // namespace eng
