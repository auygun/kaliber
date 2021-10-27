#ifndef ENGINE_MESH_H
#define ENGINE_MESH_H

#include <memory>
#include <string>

#include "engine/renderer/renderer_types.h"

namespace eng {

class Mesh {
 public:
  Mesh() = default;
  ~Mesh() = default;

  bool Create(Primitive primitive,
              const std::string& vertex_description,
              size_t num_vertices,
              const void* vertices,
              DataType index_description = kDataType_Invalid,
              size_t num_indices = 0,
              const void* indices = nullptr);

  bool Load(const std::string& file_name);

  const void* GetVertices() const { return (void*)vertices_.get(); }
  const void* GetIndices() const { return (void*)indices_.get(); }

  size_t GetVertexSize() const;
  size_t GetIndexSize() const;

  Primitive primitive() const { return primitive_; }
  const VertexDescripton& vertex_description() const {
    return vertex_description_;
  }
  size_t num_vertices() const { return num_vertices_; }
  DataType index_description() const { return index_description_; }
  size_t num_indices() const { return num_indices_; }

  bool IsValid() const { return !!vertices_.get(); }

 private:
  Primitive primitive_ = kPrimitive_TriangleStrip;
  VertexDescripton vertex_description_;
  size_t num_vertices_ = 0;
  DataType index_description_ = kDataType_Invalid;
  size_t num_indices_ = 0;
  std::unique_ptr<char[]> vertices_;
  std::unique_ptr<char[]> indices_;
};

}  // namespace eng

#endif  // ENGINE_MESH_H
