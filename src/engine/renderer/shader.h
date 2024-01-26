#ifndef ENGINE_RENDERER_SHADER_H
#define ENGINE_RENDERER_SHADER_H

#include <memory>
#include <string>

#include "base/vecmath.h"
#include "engine/renderer/render_resource.h"
#include "engine/renderer/renderer_types.h"

namespace eng {

class Renderer;
class ShaderSource;

class Shader : public RenderResource {
 public:
  Shader();
  explicit Shader(Renderer* renderer);
  ~Shader();

  Shader(Shader&& other);
  Shader& operator=(Shader&& other);

  void Create(std::unique_ptr<ShaderSource> source,
              const VertexDescription& vd,
              Primitive primitive,
              bool enable_depth_test);

  void Destroy();

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

#endif  // ENGINE_RENDERER_SHADER_H
