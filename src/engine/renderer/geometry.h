#ifndef GEOMETRY_H
#define GEOMETRY_H

#include <string>

namespace eng {

class Geometry {
public:
  Geometry() = default;
  ~Geometry();

  Geometry(const Geometry&) = delete;
  Geometry& operator=(const Geometry&) = delete;

  void Create(unsigned int primitive,
              const std::string& vertex_description,
              int num_vertices,
              const void* vertices,
              unsigned int index_description = 0,
              int num_indices = 0,
              const void* indices = NULL);
  void Destroy();

  void Draw();

  void Invalidate() { resource_id_ = 0; }
  bool IsValid() { return resource_id_ > 0; }

private:
  int resource_id_ = 0;
  static int last_id;
};

} // namespace eng

#endif // GEOMETRY_H
