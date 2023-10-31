#include "engine/asset/mesh.h"

#include <string.h>

#include "base/log.h"
#include "engine/engine.h"
#include "engine/platform/asset_file.h"
#include "third_party/jsoncpp/json.h"

namespace eng {

bool Mesh::Create(Primitive primitive,
                  const std::string& vertex_description,
                  size_t num_vertices,
                  const void* vertices,
                  DataType index_description,
                  size_t num_indices,
                  const void* indices) {
  primitive_ = primitive;
  num_vertices_ = num_vertices;
  index_description_ = index_description;
  num_indices_ = num_indices;

  if (!ParseVertexDescription(vertex_description, vertex_description_)) {
    LOG(0) << "Failed to parse vertex description.";
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
  size_t buffer_size = 0;
  auto json_mesh = AssetFile::ReadWholeFile(file_name.c_str(),
                                            Engine::Get().GetRootPath().c_str(),
                                            &buffer_size, true);
  if (!json_mesh) {
    LOG(0) << "Failed to read file: " << file_name;
    return false;
  }

  std::string err;
  Json::Value root;
  Json::CharReaderBuilder builder;
  const std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
  if (!reader->parse(json_mesh.get(), json_mesh.get() + buffer_size, &root,
                     &err)) {
    LOG(0) << "Failed to load mesh. Json parser error: " << err;
    return false;
  }

  const std::string& primitive_str = root["primitive"].asString();
  if (primitive_str == "Triangles") {
    primitive_ = kPrimitive_Triangles;
  } else if (primitive_str == "TriangleStrip") {
    primitive_ = kPrimitive_TriangleStrip;
  } else {
    LOG(0) << "Failed to load mesh. Invalid primitive: " << primitive_str;
    return false;
  }

  num_vertices_ = root["num_vertices"].asUInt();

  if (!ParseVertexDescription(root["vertex_description"].asString(),
                              vertex_description_)) {
    LOG(0) << "Failed to parse vertex description.";
    return false;
  }

  size_t array_size = 0;
  for (auto& attr : vertex_description_) {
    array_size += std::get<2>(attr);
  }
  array_size *= num_vertices_;

  const Json::Value vertices = root["vertices"];
  if (vertices.size() != array_size) {
    LOG(0) << "Failed to load mesh. Vertex array size: " << vertices.size()
           << ", expected " << array_size;
    return false;
  }

  int vertex_buffer_size = GetVertexSize() * num_vertices_;
  if (vertex_buffer_size <= 0) {
    LOG(0) << "Failed to load mesh. Invalid vertex size.";
    return false;
  }

  LOG(0) << "Loaded " << file_name
         << ". Vertex array size: " << vertices.size();

  vertices_ = std::make_unique<char[]>(vertex_buffer_size);

  char* dst = vertices_.get();
  unsigned int i = 0;
  while (i < vertices.size()) {
    for (auto& attr : vertex_description_) {
      auto [attrib_type, data_type, num_elements, type_size] = attr;
      while (num_elements--) {
        switch (data_type) {
          case kDataType_Byte:
            *((unsigned char*)dst) = (unsigned char)vertices[i].asUInt();
            break;
          case kDataType_Float:
            *((float*)dst) = (float)vertices[i].asFloat();
            break;
          case kDataType_Int:
            *((int*)dst) = vertices[i].asInt();
            break;
          case kDataType_Short:
            *((short*)dst) = (short)vertices[i].asInt();
            break;
          case kDataType_UInt:
            *((unsigned int*)dst) = vertices[i].asUInt();
            break;
          case kDataType_UShort:
            *((unsigned short*)dst) = (unsigned short)vertices[i].asUInt();
            break;
          default:
            NOTREACHED() << "- Unknown data type: " << data_type;
        }
        dst += type_size;
        ++i;
      }
    }
  }
  // TODO: Load indices
  return true;
}

size_t Mesh::GetVertexSize() const {
  return eng::GetVertexSize(vertex_description_);
}

size_t Mesh::GetIndexSize() const {
  return eng::GetIndexSize(index_description_);
}

}  // namespace eng
