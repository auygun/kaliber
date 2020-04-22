#include "../../base/file.h"
#include "../../base/log.h"
#include "ishader.h"
#include "render_command.h"
#include "../engine.h"
#include <stdlib.h>
#include <string>
#include <cstring>
#include <memory>

namespace engine {

int IShader::last_id = 0;

IShader::~IShader() {
  Destroy();
}

bool IShader::Create(const std::string& name, const std::string& vertex_description) {
  Destroy();

  auto cmd = std::make_unique<CmdCreateShader>();
  id = ++last_id;
  cmd->id = id;
  cmd->name = name;
  cmd->vertex_description = vertex_description;
  Engine::Get().GetRenderer().EnqueueCommand(std::move(cmd));

  return true;
}

void IShader::Destroy() {
  if (id) {
    auto cmd = std::make_unique<CmdDestroyShader>();
    cmd->id = id;
    Engine::Get().GetRenderer().EnqueueCommand(std::move(cmd));
    id = 0;
  }
}

void IShader::Activate() {
  if (id) {
    auto cmd = std::make_unique<CmdActivateShader>();
    cmd->id = id;
    Engine::Get().GetRenderer().EnqueueCommand(std::move(cmd));
  }
}

void IShader::SetUniform(const std::string &name, const Vector2 &v) {
  if (id) {
    auto cmd = std::make_unique<CmdSetUniformVec2>();
    cmd->id = id;
    cmd->name = name;
    cmd->v = v;
    Engine::Get().GetRenderer().EnqueueCommand(std::move(cmd));
  }
}

void IShader::SetUniform(const std::string &name, const Vector3 &v) {
  if (id) {
    auto cmd = std::make_unique<CmdSetUniformVec3>();
    cmd->id = id;
    cmd->name = name;
    cmd->v = v;
    Engine::Get().GetRenderer().EnqueueCommand(std::move(cmd));
  }
}

void IShader::SetUniform(const std::string &name, int i) {
  if (id) {
    auto cmd = std::make_unique<CmdSetUniformInt>();
    cmd->id = id;
    cmd->name = name;
    cmd->i = i;
    Engine::Get().GetRenderer().EnqueueCommand(std::move(cmd));
  }
}

} // namespace engine
