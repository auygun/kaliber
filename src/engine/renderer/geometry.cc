#include "geometry.h"
#include <stdint.h>
#include <string.h>
#include "../../base/log.h"
#include "../engine.h"
#include "../renderer/renderer.h"

namespace {

// Used to parse the vertex layout,
// e.g. "p3f;c4b" for "position 3 floats, color 4 bytes".
const char kLayoutDelimiter[] = ";/ \t";

// Map the OpenGL enumerator to the actual byte size.
unsigned GetIndexSize(GLenum type) {
  switch (type) {
    case GL_UNSIGNED_BYTE:
      return sizeof(GLbyte);
    case GL_UNSIGNED_SHORT:
      return sizeof(GLushort);
    case GL_UNSIGNED_INT:
      return sizeof(GLuint);
    default:
      return 0;
  }
}

}  // namespace

namespace engine {

Geometry::Geometry()
    : primitive_(GL_NONE),
      index_type_(GL_NONE),
      vertex_size_(0),
      vertex_array_id_(0),
      vertex_buffer_id_(0),
      index_buffer_id_(0) {}

Geometry::~Geometry() {
  Destroy();
}

bool Geometry::Create(GLenum primitive,
                      const char* vertex_description,
                      unsigned num_vertices,
                      const void* vertices,
                      GLenum index_description,
                      unsigned num_indices,
                      const void* indices) {
  // Verify that we have a valid layout and get the total byte size per vertex.
  vertex_size_ = GetVertexSize(vertex_description);
  if (!vertex_size_) {
    LOG("Invalid vertex layout\n");
    return false;
  }

  this->num_vertices_ = num_vertices;
  this->num_indices_ = num_indices;
  this->primitive_ = primitive;

  bool use_vao = Engine::Get().GetRenderer().SupportsVAO();
  if (use_vao) {
    glGenVertexArrays(1, &vertex_array_id_);
    glBindVertexArray(vertex_array_id_);
  }

  // Create the vertex buffer and upload the data.
  glGenBuffers(1, &vertex_buffer_id_);
  glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_id_);
  glBufferData(GL_ARRAY_BUFFER, num_vertices * vertex_size_, vertices,
               GL_STATIC_DRAW);

  // Make sure the vertex format is understood and the attribute pointers are
  // set up.
  if (!SetupVertexLayout(vertex_description, vertex_size_, use_vao))
    return false;

  // Create the index buffer and upload the data.
  if (indices) {
    index_type_ = index_description;

    glGenBuffers(1, &index_buffer_id_);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer_id_);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 num_indices * GetIndexSize(index_type_), indices,
                 GL_STATIC_DRAW);
  }

  if (use_vao) {
    // De-activate the buffer again and we're done.
    glBindVertexArray(0);
  } else {
    // De-activate the individual buffers.
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
  }

  return true;
}

void Geometry::Destroy() {
  if (index_buffer_id_) {
    glDeleteBuffers(1, &index_buffer_id_);
    index_buffer_id_ = 0;
  }
  if (vertex_buffer_id_) {
    glDeleteBuffers(1, &vertex_buffer_id_);
    vertex_buffer_id_ = 0;
  }
  if (vertex_array_id_) {
    glDeleteVertexArrays(1, &vertex_array_id_);
    vertex_array_id_ = 0;
  }
  vertex_layout_.clear();
}

void Geometry::Draw() {
  // Set up the vertex data.
  if (vertex_array_id_)
    glBindVertexArray(vertex_array_id_);
  else {
    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_id_);
    for (GLuint attribute_index = 0;
         attribute_index < (GLuint)vertex_layout_.size(); ++attribute_index) {
      Element& e = vertex_layout_[attribute_index];
      glEnableVertexAttribArray(attribute_index);
      glVertexAttribPointer(attribute_index, e.num_elements_, e.type_, GL_FALSE,
                            vertex_size_, (const GLvoid*)e.vertex_offset_);
    }

    if (num_indices_ > 0)
      glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer_id_);
  }

  // Draw the primitive.
  if (num_indices_ > 0)
    glDrawElements(primitive_, num_indices_, index_type_, NULL);
  else
    glDrawArrays(primitive_, 0, num_vertices_);

  // Clean up states.
  if (vertex_array_id_)
    glBindVertexArray(0);
  else {
    for (GLuint attribute_index = 0;
         attribute_index < (GLuint)vertex_layout_.size(); ++attribute_index)
      glDisableVertexAttribArray(attribute_index);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
  }
}

GLuint Geometry::GetVertexSize(const char* vertex_description) {
  GLuint size = 0;

  // Parse the description.
  char buffer[32];
  strcpy(buffer, vertex_description);
  char* token = strtok(buffer, kLayoutDelimiter);

  // Parse each encountered attribute.
  while (token) {
    // Don't care about the kind of attribute here.
    // Ignore(token[0]);

    // There can be between 1 and 4 elements in an attribute.
    size_t num_elements = token[1] - '1' + 1;
    if (num_elements < 1 || num_elements > 4)
      return 0;

    // The data type is needed, the most common ones are supported.
    size_t type_size;
    switch (token[2]) {
      case 'b':
        type_size = sizeof(GLbyte);
        break;
      case 'f':
        type_size = sizeof(GLfloat);
        break;
      case 'i':
        type_size = sizeof(GLint);
        break;
      case 's':
        type_size = sizeof(GLshort);
        break;
      case 'u':
        type_size = sizeof(GLuint);
        break;
      case 'w':
        type_size = sizeof(GLushort);
        break;
      default:
        return 0;
    }

    size += num_elements * type_size;

    token = strtok(NULL, kLayoutDelimiter);
  }

  return size;
}

bool Geometry::SetupVertexLayout(const char* vertex_description,
                                 GLuint vertex_size,
                                 bool use_vao) {
  GLuint attribute_index = 0;
  size_t vertex_offset = 0;

  // Parse the layout.
  char buffer[32];
  strcpy(buffer, vertex_description);
  char* token = strtok(buffer, kLayoutDelimiter);

  // Parse each encountered attribute.
  while (token) {
    // Check for invalid format.
    if (strlen(token) != 3)
      return false;

    // There's a limitation of 16 attributes in OpenGL ES 2.0
    if (attribute_index >= 16)
      return false;

    // Don't care about the kind of attribute here.
    // Ignore(token[0]);

    // There can be between 1 and 4 elements in an attribute.
    GLsizei num_elements = token[1] - '1' + 1;
    if (num_elements < 1 || num_elements > 4)
      return false;

    // The data type is needed, the most common ones are supported.
    GLenum type;
    size_t type_size;
    switch (token[2]) {
      case 'b':
        type = GL_UNSIGNED_BYTE;
        type_size = sizeof(GLbyte);
        break;
      case 'f':
        type = GL_FLOAT;
        type_size = sizeof(GLfloat);
        break;
      case 'i':
        type = GL_INT;
        type_size = sizeof(GLint);
        break;
      case 's':
        type = GL_SHORT;
        type_size = sizeof(GLshort);
        break;
      case 'u':
        type = GL_UNSIGNED_INT;
        type_size = sizeof(GLuint);
        break;
      case 'w':
        type = GL_UNSIGNED_SHORT;
        type_size = sizeof(GLushort);
        break;
      default:
        return false;
    }

    // We got all we need to define this attribute.
    if (use_vao) {
      // This will be saved into the vertex array object.
      glEnableVertexAttribArray(attribute_index);
      glVertexAttribPointer(attribute_index, num_elements, type, GL_FALSE,
                            vertex_size, (const GLvoid*)vertex_offset);
    } else {
      // Need to keep this information for when rendering.
      Element element;
      element.num_elements_ = num_elements;
      element.type_ = type;
      element.vertex_offset_ = vertex_offset;
      vertex_layout_.push_back(element);
    }

    // Move on to the next attribute.
    ++attribute_index;
    vertex_offset += num_elements * type_size;
    token = strtok(NULL, kLayoutDelimiter);
  }

  return true;
}

}  // namespace engine
