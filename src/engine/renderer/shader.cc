#include "shader.h"

#include "../shader_source.h"
#include "renderer.h"

using namespace base;

namespace eng {

Shader::Shader(unsigned resource_id,
               std::shared_ptr<void> impl_data,
               Renderer* renderer)
    : RenderResource(resource_id, impl_data, renderer) {}

Shader::~Shader() {
  Destroy();
}

void Shader::Create(std::unique_ptr<ShaderSource> source,
                    const VertexDescripton& vd,
                    Primitive primitive) {
  Destroy();
  valid_ = true;
  renderer_->CreateShader(impl_data_, std::move(source), vd, primitive);
}

void Shader::Destroy() {
  if (valid_) {
    renderer_->DestroyShader(impl_data_);
    valid_ = false;
  }
}

void Shader::Activate() {
  if (valid_)
    renderer_->ActivateShader(impl_data_);
}

void Shader::SetUniform(const std::string& name, const Vector2f& v) {
  if (valid_)
    renderer_->SetUniform(impl_data_, name, v);
}

void Shader::SetUniform(const std::string& name, const Vector3f& v) {
  if (valid_)
    renderer_->SetUniform(impl_data_, name, v);
}

void Shader::SetUniform(const std::string& name, const Vector4f& v) {
  if (valid_)
    renderer_->SetUniform(impl_data_, name, v);
}

void Shader::SetUniform(const std::string& name, const Matrix4f& m) {
  if (valid_)
    renderer_->SetUniform(impl_data_, name, m);
}

void Shader::SetUniform(const std::string& name, float f) {
  if (valid_)
    renderer_->SetUniform(impl_data_, name, f);
}

void Shader::SetUniform(const std::string& name, int i) {
  if (valid_)
    renderer_->SetUniform(impl_data_, name, i);
}

void Shader::UploadUniforms() {
  if (valid_)
    renderer_->UploadUniforms(impl_data_);
}

}  // namespace eng
