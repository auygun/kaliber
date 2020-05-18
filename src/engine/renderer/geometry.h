#ifndef GEOMETRY_H
#define GEOMETRY_H

#include <memory>
#include <string>

namespace eng {

class Mesh;

class Geometry {
public:
  Geometry() = default;
  ~Geometry();

  Geometry(const Geometry&) = delete;
  Geometry& operator=(const Geometry&) = delete;

  void Create(std::shared_ptr<const Mesh> mesh);
  void Destroy();

  void Draw();

  void Invalidate() { resource_id_ = 0; }
  bool IsValid() const { return resource_id_ > 0; }

  const std::string& vertex_description() const { return vertex_description_; }

private:
  int resource_id_ = 0;
  static int last_id;
  std::string vertex_description_;
};

} // namespace eng

#endif // GEOMETRY_H
