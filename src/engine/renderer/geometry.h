#ifndef GEOMETRY_H
#define GEOMETRY_H

#include "opengl.h"
#include <vector>

namespace engine {

class Geometry {
public:
  Geometry();
  ~Geometry();

  bool Create(GLenum primitive, const char *vertexDescription,
              unsigned numVertices, const void *vertices,
              GLenum indexDescription = GL_NONE, unsigned numIndices = 0,
              const void *indices = NULL);
  void Destroy();

  void Draw();


private:
  struct Element {
    GLsizei numElements;
    GLenum  type;
    size_t  vertexOffset;
  };

  unsigned              numVertices,
                        numIndices;
  GLenum                primitive,
                        indexType;
  std::vector<Element>  vertexLayout;
  size_t                vertexSize;

  GLuint                vertexArrayId,
                        vertexBufferId,
                        indexBufferId;


  GLuint GetVertexSize(const char *vertexDescription);
  bool SetupVertexLayout(const char *vertexDescription, GLuint vertexSize,
                         bool useVAO);
};

} // namespace engine

#endif // GEOMETRY_H
