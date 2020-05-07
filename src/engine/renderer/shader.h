#ifndef SHADER_H
#define SHADER_H

#include "../../base/vecmath.h"
#include "opengl.h"
#include <map>
#include <string>

namespace eng {

class Shader {
public:
  Shader() = default;
  ~Shader();

  Shader(const Shader&) = delete;
  Shader& operator=(const Shader&) = delete;

  bool Create(const std::string& name, const std::string& vertex_description);
  void Destroy();
  void Activate();

  void SetUniform(const std::string &name, const Vector2 &v);
  void SetUniform(const std::string &name, const Vector3 &v);
  void SetUniform(const std::string &name, const Vector4 &v);
  void SetUniform(const std::string &name, const Matrix4x4& m);
  void SetUniform(const std::string &name, float f);
  void SetUniform(const std::string &name, int i);

private:
  int resource_id_ = 0;
  static int last_id;
};

} // namespace eng

#endif // SHADER_H
