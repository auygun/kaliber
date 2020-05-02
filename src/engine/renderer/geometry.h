#ifndef GEOMETRY_H
#define GEOMETRY_H

#include <string>

namespace engine {

class Geometry {
public:
  Geometry() = default;
  ~Geometry();

  Geometry(const Geometry&) = delete;
  Geometry& operator=(const Geometry&) = delete;

  bool Create(unsigned int primitive,
              const std::string& vertex_description,
              int num_vertices,
              const void* vertices,
              unsigned int index_description = 0,
              int num_indices = 0,
              const void* indices = NULL);
  void Destroy();

  void Draw();

private:
  int id = 0;
  static int last_id;
};

} // namespace engine

#endif // GEOMETRY_H
