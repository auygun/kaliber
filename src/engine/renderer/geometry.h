#ifndef GEOMETRY_H
#define GEOMETRY_H

#include <memory>
#include <string>

#include "render_resource.h"
#include "renderer_types.h"

namespace eng {

class Renderer;
class Mesh;

class Geometry : public RenderResource {
 public:
  Geometry(unsigned resource_id,
           std::shared_ptr<void> impl_data,
           Renderer* renderer);
  ~Geometry() override;

  void Create(std::unique_ptr<Mesh> mesh);

  void Destroy() override;

  void Draw();

  const VertexDescripton& vertex_description() const {
    return vertex_description_;
  }

  Primitive primitive() { return primitive_; }

 private:
  VertexDescripton vertex_description_;
  Primitive primitive_ = kPrimitive_Invalid;
};

}  // namespace eng

#endif  // GEOMETRY_H
