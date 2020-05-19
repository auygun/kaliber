#ifndef MESH_H
#define MESH_H

#include "asset.h"
#include "renderer/types.h"
#include <memory>
#include <string>

namespace eng {

class Mesh : public Asset {
 public:
  static const char kLayoutDelimiter[];

  Mesh();
  ~Mesh() override;

  bool Create(Primitive primitive,
              const std::string& vertex_description,
              int num_vertices,
              const void* vertices,
              DataType index_description = kDataType_Invalid,
              int num_indices = 0,
              const void* indices = nullptr);

  bool Load(const std::string& file_name);

  unsigned int GetVertexSize() const;
  unsigned int GetIndexSize() const;

  Primitive primitive() const { return primitive_; }
  const VertexDescripton& vertex_description() const { return vertex_description_; }
  int num_vertices() const { return num_vertices_; }
  DataType index_description() const { return index_description_; }
  int num_indices() const { return num_indices_; }
  const void* vertices() const { return (void*)vertices_.get(); }
  const void* indices() const { return (void*)indices_.get(); }

  bool IsValid() const { return !!vertices_.get(); }

 protected:
  Primitive primitive_ = kPrimitive_TriangleStrip;
  VertexDescripton vertex_description_;
  int num_vertices_ = 0;
  DataType index_description_ = kDataType_Invalid;
  int num_indices_ = 0;
  std::unique_ptr<char[]> vertices_;
  std::unique_ptr<char[]> indices_;
};

} // namespace eng

#endif // MESH_H
