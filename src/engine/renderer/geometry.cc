#include "../../base/log.h"
#include "../renderer/renderer.h"
#include "../engine.h"
#include "geometry.h"
#include <stdint.h>
#include <string.h>

namespace {

// Used to parse the vertex layout,
// e.g. "p3f;c4b" for "position 3 floats, color 4 bytes".
const char kLayoutDelimiter[] = ";/ \t";

// Map the OpenGL enumerator to the actual byte size.
unsigned GetIndexSize(GLenum type) {
  switch (type) {
  case GL_UNSIGNED_BYTE:  return sizeof(GLbyte);
  case GL_UNSIGNED_SHORT: return sizeof(GLushort);
  case GL_UNSIGNED_INT:   return sizeof(GLuint);
  default:                return 0;
  }
}

} // namespace

namespace engine {

Geometry::Geometry()
  : primitive(GL_NONE)
  , indexType(GL_NONE)
  , vertexSize(0)
  , vertexArrayId(0)
  , vertexBufferId(0)
  , indexBufferId(0) {
}

Geometry::~Geometry() {
  Destroy();
}

bool Geometry::Create(GLenum primitive, const char *vertexDescription,
                      unsigned numVertices, const void *vertices,
                      GLenum indexDescription, unsigned numIndices,
                      const void *indices) {
  // Verify that we have a valid layout and get the total byte size per vertex.
  vertexSize = GetVertexSize(vertexDescription);
  if (!vertexSize) {
    LOG("Invalid vertex layout\n");
    return false;
  }

  this->numVertices = numVertices;
  this->numIndices  = numIndices;
  this->primitive   = primitive;

  bool useVAO = false;
  if (Engine::Get().GetRenderer().SupportsVAO()) {
    useVAO = true;
    glGenVertexArrays(1, &vertexArrayId);
    glBindVertexArray(vertexArrayId);
  }

  // Create the vertex buffer and upload the data.
  glGenBuffers(1, &vertexBufferId);
  glBindBuffer(GL_ARRAY_BUFFER, vertexBufferId);
  glBufferData(GL_ARRAY_BUFFER, numVertices * vertexSize, vertices,
               GL_STATIC_DRAW);

  // Make sure the vertex format is understood and the attribute pointers are
  // set up.
  if (!SetupVertexLayout(vertexDescription, vertexSize, useVAO))
    return false;

  // Create the index buffer and upload the data.
  if (indices) {
    indexType = indexDescription;

    glGenBuffers(1, &indexBufferId);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBufferId);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, numIndices * GetIndexSize(indexType),
                 indices, GL_STATIC_DRAW);
  }

  if (useVAO) {
    // De-activate the buffer again and we're done.
    glBindVertexArray(0);
  } else {
    // De-activate the individual buffers.
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
  }

  return false;
}

void Geometry::Destroy() {
  if (indexBufferId) {
    glDeleteBuffers(1, &indexBufferId);
    indexBufferId = 0;
  }
  if (vertexBufferId) {
    glDeleteBuffers(1, &vertexBufferId);
    vertexBufferId = 0;
  }
  if (vertexArrayId) {
    glDeleteVertexArrays(1, &vertexArrayId);
    vertexArrayId = 0;
  }
  vertexLayout.clear();
}

void Geometry::Draw() {
  // Set up the vertex data.
  if (vertexArrayId)
    glBindVertexArray(vertexArrayId);
  else {
    glBindBuffer(GL_ARRAY_BUFFER, vertexBufferId);
    for (GLuint attributeIndex = 0; attributeIndex < (GLuint)vertexLayout.size();
         ++attributeIndex) {
      Element &e = vertexLayout[attributeIndex];
      glEnableVertexAttribArray(attributeIndex);
      glVertexAttribPointer(attributeIndex, e.numElements, e.type, GL_FALSE,
                            vertexSize, (const GLvoid *)e.vertexOffset);
    }

    if (numIndices > 0)
      glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBufferId);
  }

  // Draw the primitive.
  if (numIndices > 0)
    glDrawElements(primitive, numIndices, indexType, NULL);
  else
    glDrawArrays(primitive, 0, numVertices);

  // Clean up states.
  if (vertexArrayId)
    glBindVertexArray(0);
  else {
    for (GLuint attributeIndex = 0; attributeIndex < (GLuint)vertexLayout.size();
         ++attributeIndex)
      glDisableVertexAttribArray(attributeIndex);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
  }
}

GLuint Geometry::GetVertexSize(const char *vertexDescription) {
  GLuint size = 0;

  // Parse the description.
  char buffer[32];
  strcpy(buffer, vertexDescription);
  char *token = strtok(buffer, kLayoutDelimiter);

  // Parse each encountered attribute.
  while (token) {
    // Don't care about the kind of attribute here.
    // Ignore(token[0]);

    // There can be between 1 and 4 elements in an attribute.
    size_t numElements = token[1] - '1' + 1;
    if (numElements < 1 || numElements > 4)
      return 0;

    // The data type is needed, the most common ones are supported.
    size_t typeSize;
    switch (token[2]) {
    case 'b': typeSize = sizeof(GLbyte);    break;
    case 'f': typeSize = sizeof(GLfloat);   break;
    case 'i': typeSize = sizeof(GLint);     break;
    case 's': typeSize = sizeof(GLshort);   break;
    case 'u': typeSize = sizeof(GLuint);    break;
    case 'w': typeSize = sizeof(GLushort);  break;
    default:  return 0;
    }

    size += numElements * typeSize;

    token = strtok(NULL, kLayoutDelimiter);
  }

  return size;
}

bool Geometry::SetupVertexLayout(const char *vertexDescription,
                                 GLuint vertexSize, bool useVAO) {
  GLuint attributeIndex = 0;
  size_t vertexOffset = 0;

  // Parse the layout.
  char buffer[32];
  strcpy(buffer, vertexDescription);
  char *token = strtok(buffer, kLayoutDelimiter);

  // Parse each encountered attribute.
  while (token) {
    // Check for invalid format.
    if (strlen(token) != 3)
      return false;

    // There's a limitation of 16 attributes in OpenGL ES 2.0
    if (attributeIndex >= 16)
      return false;

    // Don't care about the kind of attribute here.
    // Ignore(token[0]);

    // There can be between 1 and 4 elements in an attribute.
    GLsizei numElements = token[1] - '1' + 1;
    if (numElements < 1 || numElements > 4)
      return false;

    // The data type is needed, the most common ones are supported.
    GLenum type;
    size_t typeSize;
    switch (token[2]) {
    case 'b': type = GL_UNSIGNED_BYTE;  typeSize = sizeof(GLbyte);    break;
    case 'f': type = GL_FLOAT;          typeSize = sizeof(GLfloat);   break;
    case 'i': type = GL_INT;            typeSize = sizeof(GLint);     break;
    case 's': type = GL_SHORT;          typeSize = sizeof(GLshort);   break;
    case 'u': type = GL_UNSIGNED_INT;   typeSize = sizeof(GLuint);    break;
    case 'w': type = GL_UNSIGNED_SHORT; typeSize = sizeof(GLushort);  break;
    default:  return false;
    }

    // We got all we need to define this attribute.
    if (useVAO) {
      // This will be saved into the vertex array object.
      glEnableVertexAttribArray(attributeIndex);
      glVertexAttribPointer(attributeIndex, numElements, type, GL_FALSE,
                            vertexSize, (const GLvoid *)vertexOffset);
    } else {
      // Need to keep this information for when rendering.
      Element element;
      element.numElements   = numElements;
      element.type          = type;
      element.vertexOffset  = vertexOffset;
      vertexLayout.push_back(element);
    }

    // Move on to the next attribute.
    ++attributeIndex;
    vertexOffset += numElements * typeSize;
    token = strtok(NULL, kLayoutDelimiter);
  }

  return true;
}

} // namespace engine
