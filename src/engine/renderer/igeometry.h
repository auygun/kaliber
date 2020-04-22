#ifndef IGEOMETRY_H
#define IGEOMETRY_H

#include <string>

namespace engine {

class IGeometry {
public:
  IGeometry() = default;
  ~IGeometry();

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

#endif // IGEOMETRY_H
