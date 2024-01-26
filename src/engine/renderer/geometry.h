#ifndef ENGINE_RENDERER_GEOMETRY_H
#define ENGINE_RENDERER_GEOMETRY_H

#include <memory>
#include <string>

#include "engine/renderer/render_resource.h"
#include "engine/renderer/renderer_types.h"

namespace eng {

class Renderer;
class Mesh;

class Geometry : public RenderResource {
 public:
  Geometry();
  explicit Geometry(Renderer* renderer);
  ~Geometry();

  Geometry(Geometry&& other);
  Geometry& operator=(Geometry&& other);

  void Create(std::unique_ptr<Mesh> mesh);
  void Create(Primitive primitive,
              VertexDescription vertex_description,
              DataType index_description = kDataType_Invalid);

  void Update(size_t num_vertices,
              const void* vertices,
              size_t num_indices,
              const void* indices);

  void Destroy();

  void Draw();
  void Draw(uint64_t num_indices, uint64_t start_offset);

  const VertexDescription& vertex_description() const {
    return vertex_description_;
  }

  Primitive primitive() { return primitive_; }

 private:
  VertexDescription vertex_description_;
  Primitive primitive_ = kPrimitive_Invalid;
};

}  // namespace eng

#endif  // ENGINE_RENDERER_GEOMETRY_H
