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

  void Create(std::unique_ptr<ShaderSource> source,
              const VertexDescripton& vd,
              Primitive primitive);

  void Destroy() override;

  void Activate();

  void SetUniform(const std::string& name, const base::Vector2f& v);
  void SetUniform(const std::string& name, const base::Vector3f& v);
  void SetUniform(const std::string& name, const base::Vector4f& v);
  void SetUniform(const std::string& name, const base::Matrix4f& m);
  void SetUniform(const std::string& name, float f);
  void SetUniform(const std::string& name, int i);

  void UploadUniforms();
};

}  // namespace eng

#endif  // SHADER_H
