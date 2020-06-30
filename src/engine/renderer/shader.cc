#include "shader.h"

#include "../shader_source.h"
#include "render_command.h"
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
                    const VertexDescripton& vd) {
  Destroy();
  valid_ = true;

  auto cmd = std::make_unique<CmdCreateShader>();
  cmd->source = std::move(source);
  cmd->vertex_description = vd;
  cmd->impl_data = impl_data_;
  renderer_->EnqueueCommand(std::move(cmd));
}

void Shader::Destroy() {
  if (valid_) {
    auto cmd = std::make_unique<CmdDestroyShader>();
    cmd->impl_data = impl_data_;
    renderer_->EnqueueCommand(std::move(cmd));
    valid_ = false;
  }
}

void Shader::Activate() {
  if (valid_) {
    auto cmd = std::make_unique<CmdActivateShader>();
    cmd->impl_data = impl_data_;
    renderer_->EnqueueCommand(std::move(cmd));
  }
}

void Shader::SetUniform(const std::string& name, const Vector2& v) {
  if (valid_) {
    auto cmd = std::make_unique<CmdSetUniformVec2>();
    cmd->name = name;
    cmd->v = v;
    cmd->impl_data = impl_data_;
    renderer_->EnqueueCommand(std::move(cmd));
  }
}

void Shader::SetUniform(const std::string& name, const Vector3& v) {
  if (valid_) {
    auto cmd = std::make_unique<CmdSetUniformVec3>();
    cmd->name = name;
    cmd->v = v;
    cmd->impl_data = impl_data_;
    renderer_->EnqueueCommand(std::move(cmd));
  }
}

void Shader::SetUniform(const std::string& name, const Vector4& v) {
  if (valid_) {
    auto cmd = std::make_unique<CmdSetUniformVec4>();
    cmd->name = name;
    cmd->v = v;
    cmd->impl_data = impl_data_;
    renderer_->EnqueueCommand(std::move(cmd));
  }
}

void Shader::SetUniform(const std::string& name, const Matrix4x4& m) {
  if (valid_) {
    auto cmd = std::make_unique<CmdSetUniformMat4>();
    cmd->name = name;
    cmd->m = m;
    cmd->impl_data = impl_data_;
    renderer_->EnqueueCommand(std::move(cmd));
  }
}

void Shader::SetUniform(const std::string& name, float f) {
  if (valid_) {
    auto cmd = std::make_unique<CmdSetUniformFloat>();
    cmd->name = name;
    cmd->f = f;
    cmd->impl_data = impl_data_;
    renderer_->EnqueueCommand(std::move(cmd));
  }
}

void Shader::SetUniform(const std::string& name, int i) {
  if (valid_) {
    auto cmd = std::make_unique<CmdSetUniformInt>();
    cmd->name = name;
    cmd->i = i;
    cmd->impl_data = impl_data_;
    renderer_->EnqueueCommand(std::move(cmd));
  }
}

}  // namespace eng
