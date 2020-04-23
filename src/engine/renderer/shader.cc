#include "../../base/file.h"
#include "../../base/log.h"
#include "shader.h"
#include "render_command.h"
#include "../engine.h"
#include <stdlib.h>
#include <string>
#include <cstring>
#include <memory>

namespace engine {

int Shader::last_id = 0;

Shader::~Shader() {
  Destroy();
}

bool Shader::Create(const std::string& name, const std::string& vertex_description) {
  Destroy();

  std::unique_ptr<char[]> vertexSource;
  std::unique_ptr<char[]> fragmentSource;

  std::string vertexFileName = name;
  vertexFileName += "_vertex.glsl";
  vertexSource.reset(File::ReadWholeFile(vertexFileName.c_str(), NULL, true));
  if (!vertexSource)
    return false;

  std::string fragmentFileName = name;
  fragmentFileName += "_fragment.glsl";
  fragmentSource.reset(File::ReadWholeFile(fragmentFileName.c_str(), NULL, true));
  if (!fragmentSource)
    return false;

  auto cmd = std::make_unique<CmdCreateShader>();
  id = ++last_id;
  cmd->id = id;
  cmd->fragment_source = std::move(fragmentSource);
  cmd->vertex_source = std::move(vertexSource);
  cmd->vertex_description = vertex_description;
  Engine::Get().GetRenderer().EnqueueCommand(std::move(cmd));

  return true;
}

void Shader::Destroy() {
  if (id) {
    auto cmd = std::make_unique<CmdDestroyShader>();
    cmd->id = id;
    Engine::Get().GetRenderer().EnqueueCommand(std::move(cmd));
    id = 0;
  }
}

void Shader::Activate() {
  if (id) {
    auto cmd = std::make_unique<CmdActivateShader>();
    cmd->id = id;
    Engine::Get().GetRenderer().EnqueueCommand(std::move(cmd));
  }
}

void Shader::SetUniform(const std::string &name, const Vector2 &v) {
  if (id) {
    auto cmd = std::make_unique<CmdSetUniformVec2>();
    cmd->id = id;
    cmd->name = name;
    cmd->v = v;
    Engine::Get().GetRenderer().EnqueueCommand(std::move(cmd));
  }
}

void Shader::SetUniform(const std::string &name, const Vector3 &v) {
  if (id) {
    auto cmd = std::make_unique<CmdSetUniformVec3>();
    cmd->id = id;
    cmd->name = name;
    cmd->v = v;
    Engine::Get().GetRenderer().EnqueueCommand(std::move(cmd));
  }
}

void Shader::SetUniform(const std::string &name, int i) {
  if (id) {
    auto cmd = std::make_unique<CmdSetUniformInt>();
    cmd->id = id;
    cmd->name = name;
    cmd->i = i;
    Engine::Get().GetRenderer().EnqueueCommand(std::move(cmd));
  }
}

} // namespace engine
