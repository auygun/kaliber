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
                    bool enable_depth_test,
                    CullMode cull_mode) {
  Destroy();
  resource_id_ = renderer_->CreateShader(std::move(source), vd, primitive,
                                         enable_depth_test, cull_mode);
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

}  // namespace eng
