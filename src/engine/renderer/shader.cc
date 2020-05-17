#include "shader.h"
#include "../../base/file.h"
#include "../../base/log.h"
#include "render_command.h"
#include "../engine.h"
#include "../shader_code.h"
#include <cstring>

namespace eng {

int Shader::last_id = 0;

Shader::~Shader() {
  Destroy();
}

bool Shader::Create(std::shared_ptr<const ShaderCode> code,
                    const std::string& vertex_description) {
  if (!code->IsImmutable()) {
    DLOG << "Cannot create shader from mutable code.";
    return false;
  }

  Destroy();

  auto cmd = std::make_unique<CmdCreateShader>();
  resource_id_ = ++last_id;
  cmd->id = resource_id_;
  cmd->code = code;
  cmd->vertex_description = vertex_description;
  Engine::Get().EnqueueRenderCommand(std::move(cmd));

  return true;
}

void Shader::Destroy() {
  if (resource_id_) {
    auto cmd = std::make_unique<CmdDestroyShader>();
    cmd->id = resource_id_;
    Engine::Get().EnqueueRenderCommand(std::move(cmd));
    resource_id_ = 0;
  }
}

void Shader::Activate() {
  if (resource_id_) {
    auto cmd = std::make_unique<CmdActivateShader>();
    cmd->id = resource_id_;
    Engine::Get().EnqueueRenderCommand(std::move(cmd));
  }
}

void Shader::SetUniform(const std::string &name, const Vector2 &v) {
  if (resource_id_) {
    auto cmd = std::make_unique<CmdSetUniformVec2>();
    cmd->id = resource_id_;
    cmd->name = name;
    cmd->v = v;
    Engine::Get().EnqueueRenderCommand(std::move(cmd));
  }
}

void Shader::SetUniform(const std::string &name, const Vector3 &v) {
  if (resource_id_) {
    auto cmd = std::make_unique<CmdSetUniformVec3>();
    cmd->id = resource_id_;
    cmd->name = name;
    cmd->v = v;
    Engine::Get().EnqueueRenderCommand(std::move(cmd));
  }
}

void Shader::SetUniform(const std::string &name, const Vector4 &v) {
  if (resource_id_) {
    auto cmd = std::make_unique<CmdSetUniformVec4>();
    cmd->id = resource_id_;
    cmd->name = name;
    cmd->v = v;
    Engine::Get().EnqueueRenderCommand(std::move(cmd));
  }
}

void Shader::SetUniform(const std::string &name, const Matrix4x4& m) {
  if (resource_id_) {
    auto cmd = std::make_unique<CmdSetUniformMat4>();
    cmd->id = resource_id_;
    cmd->name = name;
    cmd->m = m;
    Engine::Get().EnqueueRenderCommand(std::move(cmd));
  }
}

void Shader::SetUniform(const std::string &name, float f) {
  if (resource_id_) {
    auto cmd = std::make_unique<CmdSetUniformFloat>();
    cmd->id = resource_id_;
    cmd->name = name;
    cmd->f = f;
    Engine::Get().EnqueueRenderCommand(std::move(cmd));
  }
}

void Shader::SetUniform(const std::string &name, int i) {
  if (resource_id_) {
    auto cmd = std::make_unique<CmdSetUniformInt>();
    cmd->id = resource_id_;
    cmd->name = name;
    cmd->i = i;
    Engine::Get().EnqueueRenderCommand(std::move(cmd));
  }
}

} // namespace eng
