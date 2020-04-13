#ifndef MESH_H
#define MESH_H

#include <memory>
#include <string>
#include "asset.h"
#include "renderer/renderer_types.h"

namespace eng {

class Mesh : public Asset {
 public:
  static const char kLayoutDelimiter[];

  Mesh() = default;
  ~Mesh() override = default;

  bool Create(Primitive primitive,
              const std::string& vertex_description,
              size_t num_vertices,
              const void* vertices,
              DataType index_description = kDataType_Invalid,
              size_t num_indices = 0,
              const void* indices = nullptr);

  bool Load(const std::string& file_name) override;

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

 protected:
  Primitive primitive_ = kPrimitive_TriangleStrip;
  VertexDescripton vertex_description_;
  size_t num_vertices_ = 0;
  DataType index_description_ = kDataType_Invalid;
  size_t num_indices_ = 0;
  std::unique_ptr<char[]> vertices_;
  std::unique_ptr<char[]> indices_;
};

}  // namespace eng

#endif  // MESH_H
