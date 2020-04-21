#ifndef GEOMETRY_H
#define GEOMETRY_H

#include <vector>
#include <memory>
#include <string>
#include "opengl.h"

namespace engine {

class Geometry {
 public:
  Geometry();
  ~Geometry();

  bool Create(GLenum primitive,
              const std::string& vertex_description,
              int num_vertices,
              const void* vertices,
              GLenum index_description = GL_NONE,
              int num_indices = 0,
              const void* indices = NULL);
  void Destroy();

  void Draw();

 private:
  struct Element {
    GLsizei num_elements_;
    GLenum type_;
    size_t vertex_offset_;
  };

  int num_vertices_;
  int num_indices_;
  GLenum primitive_;
  GLenum index_type_;
  std::vector<Element> vertex_layout_;
  size_t vertex_size_;

  GLuint vertex_array_id_;
  GLuint vertex_buffer_id_;
  GLuint index_buffer_id_;

  GLuint GetVertexSize(const std::string& vertex_description);
  bool SetupVertexLayout(const std::string& vertex_description,
                         GLuint vertex_size,
                         bool use_vao);
};

}  // namespace engine

#endif  // GEOMETRY_H
