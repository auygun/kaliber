#ifndef SHADER_H
#define SHADER_H

#include "../../base/vecmath.h"
#include <string>
#include <memory>

namespace eng {

class ShaderCode;;

class Shader {
public:
  Shader() = default;
  ~Shader();

  Shader(const Shader&) = delete;
  Shader& operator=(const Shader&) = delete;

  bool Create(std::shared_ptr<const ShaderCode> code,
              const std::string& vertex_description);
  void Destroy();
  void Activate();

  void SetUniform(const std::string &name, const base::Vector2 &v);
  void SetUniform(const std::string &name, const base::Vector3 &v);
  void SetUniform(const std::string &name, const base::Vector4 &v);
  void SetUniform(const std::string &name, const base::Matrix4x4& m);
  void SetUniform(const std::string &name, float f);
  void SetUniform(const std::string &name, int i);

  void Invalidate() { resource_id_ = 0; }
  bool IsValid() const { return resource_id_ > 0; }

private:
  int resource_id_ = 0;
  static int last_id;
};

} // namespace eng

#endif // SHADER_H
