#include "engine/renderer/shader.h"

#include "engine/asset/shader_source.h"
#include "engine/renderer/renderer.h"

using namespace base;

namespace eng {

Shader::Shader() : RenderResource(nullptr) {}

Shader::Shader(Renderer* renderer) : RenderResource(renderer) {}

Shader::~Shader() {
  Destroy();
}

Shader::Shader(Shader&& other) {
  Move(other);
}

Shader& Shader::operator=(Shader&& other) {
  Destroy();
  Move(other);
  return *this;
}

void Shader::Create(std::unique_ptr<ShaderSource> source,
                    const VertexDescription& vd,
                    Primitive primitive,
                    bool enable_depth_test) {
  Destroy();
  resource_id_ = renderer_->CreateShader(std::move(source), vd, primitive,
                                         enable_depth_test);
}

void Shader::Destroy() {
  if (IsValid()) {
    renderer_->DestroyShader(resource_id_);
    resource_id_ = 0;
  }
}

void Shader::Activate() {
  if (IsValid())
    renderer_->ActivateShader(resource_id_);
}

void Shader::SetUniform(const std::string& name, const Vector2f& v) {
  if (IsValid())
    renderer_->SetUniform(resource_id_, name, v);
}

void Shader::SetUniform(const std::string& name, const Vector3f& v) {
  if (IsValid())
    renderer_->SetUniform(resource_id_, name, v);
}

void Shader::SetUniform(const std::string& name, const Vector4f& v) {
  if (IsValid())
    renderer_->SetUniform(resource_id_, name, v);
}

void Shader::SetUniform(const std::string& name, const Matrix4f& m) {
  if (IsValid())
    renderer_->SetUniform(resource_id_, name, m);
}

void Shader::SetUniform(const std::string& name, float f) {
  if (IsValid())
    renderer_->SetUniform(resource_id_, name, f);
}

void Shader::SetUniform(const std::string& name, int i) {
  if (IsValid())
    renderer_->SetUniform(resource_id_, name, i);
}

}  // namespace eng
