#ifndef SHADER_H
#define SHADER_H

#include <memory>
#include <string>

#include "../../base/vecmath.h"
#include "render_resource.h"
#include "types.h"

namespace eng {

class ShaderSource;

class Shader : public RenderResource {
 public:
  Shader() = default;
  ~Shader() override;

  void Create(std::shared_ptr<const ShaderSource> source,
              const VertexDescripton& vd);
  void Destroy();
  void Activate();

  void SetUniform(const std::string& name, const base::Vector2& v);
  void SetUniform(const std::string& name, const base::Vector3& v);
  void SetUniform(const std::string& name, const base::Vector4& v);
  void SetUniform(const std::string& name, const base::Matrix4x4& m);
  void SetUniform(const std::string& name, float f);
  void SetUniform(const std::string& name, int i);

 private:
  static int last_id;
};

}  // namespace eng

#endif  // SHADER_H
