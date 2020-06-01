#ifndef GEOMETRY_H
#define GEOMETRY_H

#include <memory>
#include <string>

#include "render_resource.h"
#include "types.h"

namespace eng {

class Mesh;

class Geometry : public RenderResource {
 public:
  Geometry(unsigned resource_id);
  ~Geometry() override;

  void Create(std::shared_ptr<const Mesh> mesh);
  void Destroy();

  void Draw();

  const VertexDescripton& vertex_description() const {
    return vertex_description_;
  }

 private:
  VertexDescripton vertex_description_;
};

}  // namespace eng

#endif  // GEOMETRY_H
