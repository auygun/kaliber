#include "mesh.h"
#include <string.h>
#include <array>
#include <cassert>
#include <utility>
#include "../base/asset_file.h"
#include "../base/log.h"
#include "../third_party/json11/json11.h"
#include "engine.h"

using json11::Json;

namespace eng {

// Used to parse the vertex layout,
// e.g. "p3f;c4b" for "position 3 floats, color 4 bytes".
const char Mesh::kLayoutDelimiter[] = ";/ \t";

Mesh::Mesh() = default;
Mesh::~Mesh() = default;

bool Mesh::Create(Primitive primitive,
                  const std::string& vertex_description,
                  size_t num_vertices,
                  const void* vertices,
                  DataType index_description,
                  size_t num_indices,
                  const void* indices) {
  if (IsImmutable()) {
    LOG << "Error: Mesh is immutable. Failed to create.";
    return false;
  }

  primitive_ = primitive;
  num_vertices_ = num_vertices;
  index_description_ = index_description;
  num_indices_ = num_indices;

  if (!ParseVertexDescription(vertex_description, vertex_description_)) {
    LOG << "Failed to parse vertex description.";
    return false;
  }

  int vertex_buffer_size = GetVertexSize() * num_vertices_;
  if (vertex_buffer_size > 0) {
    vertices_ = std::make_unique<char[]>(vertex_buffer_size);
    memcpy(vertices_.get(), vertices, vertex_buffer_size);
  }

  if (!indices)
    return true;

  int index_buffer_size = GetIndexSize() * num_indices_;
  if (index_buffer_size > 0) {
    indices_ = std::make_unique<char[]>(index_buffer_size);
    memcpy(indices_.get(), indices, index_buffer_size);
  }

  return true;
}

bool Mesh::Load(const std::string& file_name) {
  if (IsImmutable()) {
    LOG << "Error: Mesh is immutable. Failed to load.";
    return false;
  }

  SetName(file_name);

  std::unique_ptr<char[]> json_mesh = base::AssetFile::ReadWholeFile(
      file_name.c_str(), Engine::Get().GetRootPath().c_str(), NULL, true);
  if (!json_mesh) {
    LOG << "Failed to read file: " << file_name;
    return false;
  }

  std::string err;
  const auto json = Json::parse(json_mesh.get(), err, json11::COMMENTS);
  if (!err.empty()) {
    LOG << "Failed to load mesh. Json parser error: " << err;
    return false;
  }

  const std::string& primitive_str = json["primitive"].string_value();
  if (primitive_str == "Triangles") {
    primitive_ = kPrimitive_Triangles;
  } else if (primitive_str == "TriangleStrip") {
    primitive_ = kPrimitive_TriangleStrip;
  } else {
    LOG << "Failed to load mesh. Invalid primitive: " << primitive_str;
    return false;
  }

  num_vertices_ = json["num_vertices"].int_value();

  if (!ParseVertexDescription(json["vertex_description"].string_value(),
                              vertex_description_)) {
    LOG << "Failed to parse vertex description.";
    return false;
  }

  size_t array_size = 0;
  for (auto& attr : vertex_description_) {
    array_size += std::get<2>(attr);
  }
  array_size *= num_vertices_;

  const Json::array& vertices = json["vertices"].array_items();
  if (vertices.size() != array_size) {
    LOG << "Failed to load mesh. Vertex array size: " << vertices.size()
        << ", expected " << array_size;
    return false;
  }

  int vertex_buffer_size = GetVertexSize() * num_vertices_;
  if (vertex_buffer_size <= 0) {
    LOG << "Failed to load mesh. Invalid vertex size.";
    return false;
  }

  vertices_ = std::make_unique<char[]>(vertex_buffer_size);

  char* dst = vertices_.get();
  auto it = vertices.begin();
  while (it != vertices.end()) {
    for (auto& attr : vertex_description_) {
      auto [attrib_type, data_type, num_elements, type_size] = attr;
      while (num_elements--) {
        switch (data_type) {
          case kDataType_Byte:
            *((unsigned char*)dst) = (unsigned char)it->int_value();
            break;
          case kDataType_Float:
            *((float*)dst) = (float)it->number_value();
            break;
          case kDataType_Int:
            *((int*)dst) = it->int_value();
            break;
          case kDataType_Short:
            *((short*)dst) = (short)it->int_value();
            break;
          case kDataType_UInt:
            *((unsigned int*)dst) = (unsigned int)it->number_value();
            break;
          case kDataType_UShort:
            *((unsigned short*)dst) = (unsigned short)it->int_value();
            break;
          default:
            assert(false);
            return false;
        }
        dst += type_size;
        ++it;
      }
    }
  }
  return true;
}

size_t Mesh::GetVertexSize() const {
  unsigned int size = 0;
  for (auto& attr : vertex_description_) {
    size += std::get<2>(attr) * std::get<3>(attr);
  }
  return size;
}

size_t Mesh::GetIndexSize() const {
  switch (index_description_) {
    case kDataType_Byte:
      return sizeof(char);
    case kDataType_UShort:
      return sizeof(unsigned short);
    case kDataType_UInt:
      return sizeof(unsigned int);
    default:
      return 0;
  }
}

}  // namespace eng
