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
  Geometry() = default;
  ~Geometry() override;

  void Create(std::shared_ptr<const Mesh> mesh);
  void Destroy();

  void Draw();

  const VertexDescripton& vertex_description() const {
    return vertex_description_;
  }

 private:
  static int last_id;
  VertexDescripton vertex_description_;

  Geometry(const Geometry&) = delete;
  Geometry& operator=(const Geometry&) = delete;
};

}  // namespace eng

#endif  // GEOMETRY_H
