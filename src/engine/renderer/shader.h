#ifndef SHADER_H
#define SHADER_H

#include <memory>
#include <string>

#include "../../base/vecmath.h"
#include "render_resource.h"
#include "renderer_types.h"

namespace eng {

class Renderer;
class ShaderSource;

class Shader : public RenderResource {
 public:
  Shader(unsigned resource_id,
         std::shared_ptr<void> impl_data,
         Renderer* renderer);
  ~Shader() override;

  void Create(std::unique_ptr<ShaderSource> source, const VertexDescripton& vd);

  void Destroy() override;

  void Activate();

  void SetUniform(const std::string& name, const base::Vector2& v);
  void SetUniform(const std::string& name, const base::Vector3& v);
  void SetUniform(const std::string& name, const base::Vector4& v);
  void SetUniform(const std::string& name, const base::Matrix4x4& m);
  void SetUniform(const std::string& name, float f);
  void SetUniform(const std::string& name, int i);
};

}  // namespace eng

#endif  // SHADER_H
