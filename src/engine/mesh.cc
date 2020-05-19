#include "mesh.h"
#include "engine.h"
#include "../base/asset_file.h"
#include "../base/log.h"
#include "../third_party/json11/json11.h"
#include <cassert>
#include <string.h>
#include <array>
#include <utility>

using json11::Json;

namespace eng {

// Used to parse the vertex layout,
// e.g. "p3f;c4b" for "position 3 floats, color 4 bytes".
const char Mesh::kLayoutDelimiter[] = ";/ \t";

Mesh::Mesh() = default;
Mesh::~Mesh() = default;

void Mesh::Create(Primitive primitive,
                  const std::string& vertex_description,
                  int num_vertices,
                  const void* vertices,
                  DataType index_description,
                  int num_indices,
                  const void* indices) {
  if (IsImmutable()) {
    LOG << "Error: Mesh is immutable. Failed to create.";
    return;
  }

  primitive_ = primitive;
  vertex_description_ = vertex_description;
  num_vertices_ = num_vertices;
  index_description_ = index_description;
  num_indices_ = num_indices;

  int vertex_buffer_size = GetVertexSize() * num_vertices_;
  if (vertex_buffer_size > 0) {
    vertices_ = std::make_unique<char[]>(vertex_buffer_size);
    memcpy(vertices_.get(), vertices, vertex_buffer_size);
  }

  if (!indices)
    return;

  int index_buffer_size = GetIndexSize() * num_indices_;
  if (index_buffer_size > 0) {
    indices_ = std::make_unique<char[]>(index_buffer_size);
    memcpy(indices_.get(), indices, index_buffer_size);
  }
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
    primitive_ = kTriangles;
  } else if (primitive_str == "TriangleStrip") {
    primitive_ = kTriangleStrip;
  } else {
    LOG << "Failed to load mesh. Invalid primitive: " << primitive_str;
    return false;
  }

  vertex_description_ = json["vertex_description"].string_value();
  num_vertices_ = json["num_vertices"].int_value();

  size_t array_size = 0;
  std::vector<std::pair<char, size_t>> vertex_elements;
  ParseVertexDescription([&](char* token,
                            size_t num_elements,
                            size_t type_size)->bool {
    vertex_elements.push_back({token[2], num_elements});
    array_size += num_elements;
    return true;
  });
  array_size *= num_vertices_;

  const Json::array& vertices = json["vertices"].array_items();
  if (vertices.size() != array_size) {
    LOG << "Failed to load mesh. Vertex array size: " << vertices.size() <<
        ", expected " << array_size;
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
    for (auto& ve : vertex_elements) {
      size_t num_elements = ve.second;
      switch (ve.first) {
      case 'b':
        while (num_elements--) {
          *dst = (char)it->int_value();
          dst += sizeof(char);
          ++it;
        }
        break;
      case 'f':
        while (num_elements--) {
          *((float*)dst) = (float)it->number_value();
          dst += sizeof(float);
          ++it;
        }
        break;
      case 'i':
        while (num_elements--) {
          *((int*)dst) = it->number_value();
          dst += sizeof(int);
          ++it;
        }
        break;
      case 's':
        while (num_elements--) {
          *((short*)dst) = (short)it->number_value();
          dst += sizeof(short);
          ++it;
        }
        break;
      case 'u':
        while (num_elements--) {
          *((unsigned int*)dst) = (unsigned int)it->number_value();
          dst += sizeof(unsigned int);
          ++it;
        }
        break;
      case 'w':
        while (num_elements--) {
          *((unsigned short*)dst) = (unsigned short)it->number_value();
          dst += sizeof(unsigned short);
          ++it;
        }
        break;
      default:
        assert(false);
        return false;
      }
    }
  }
  return true;
}

unsigned int Mesh::GetVertexSize() const {
  unsigned int size = 0;
  ParseVertexDescription([&](char* token,
                            size_t num_elements,
                            size_t type_size)->bool {
    size += num_elements * type_size;
    return true;
  });
  return size;
}

unsigned int Mesh::GetIndexSize() const {
  switch (index_description_) {
  case kByte:
    return sizeof(char);
  case kUShort:
    return sizeof(unsigned short);
  case kUInt:
    return sizeof(unsigned int);
  default:
    return 0;
  }
}

bool Mesh::ParseVertexDescription(
    std::function<bool(char*, size_t, size_t)> cb) const {
  // Parse the description.
  char buffer[32];
  strcpy(buffer, vertex_description_.c_str());
  char *token = strtok(buffer, kLayoutDelimiter);

  // Parse each encountered attribute.
  while (token) {
    // Check for invalid format.
    if (strlen(token) != 3)
      return false;

    // Validate the kind of attribute.
    if (token[0] != 'c' && token[0] != 'n' && token[0] != 'p' &&
        token[0] != 't')
      return false;

    // There can be between 1 and 4 elements in an attribute.
    size_t num_elements = token[1] - '1' + 1;
    if (num_elements < 1 || num_elements > 4)
      return false;

    // The data type is needed, the most common ones are supported.
    size_t type_size;
    switch (token[2]) {
    case 'b':
      type_size = sizeof(char);
      break;
    case 'f':
      type_size = sizeof(float);
      break;
    case 'i':
      type_size = sizeof(int);
      break;
    case 's':
      type_size = sizeof(short);
      break;
    case 'u':
      type_size = sizeof(unsigned int);
      break;
    case 'w':
      type_size = sizeof(unsigned short);
      break;
    default:
      return false;
    }

    if (!cb(token, num_elements, type_size))
      return false;

    token = strtok(NULL, kLayoutDelimiter);
  }
  return true;
}

} // namespace eng
