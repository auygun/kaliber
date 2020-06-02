#ifndef RENDER_COMMAND_H
#define RENDER_COMMAND_H

#include <array>
#include <memory>
#include <string>
#include "../../base/hash.h"
#include "../../base/vecmath.h"
#include "renderer_types.h"

namespace eng {

class Image;
class ShaderSource;
class Mesh;

// Global render commands are guaranteed to be processed. Others commands are
// frame specific and can be discared by the renderer.

#ifdef _DEBUG
#define RENDER_COMMAND_BEGIN(NAME, GLOBAL)            \
  struct NAME : RenderCommand {                       \
    static constexpr CommandId CMD_ID = HHASH(#NAME); \
    NAME() : RenderCommand(CMD_ID, GLOBAL, #NAME) {}
#define RENDER_COMMAND_END };
#else
#define RENDER_COMMAND_BEGIN(NAME, GLOBAL)            \
  struct NAME : RenderCommand {                       \
    static constexpr CommandId CMD_ID = HHASH(#NAME); \
    NAME() : RenderCommand(CMD_ID, GLOBAL) {}
#define RENDER_COMMAND_END };
#endif

struct RenderCommand {
  using CommandId = size_t;
  static constexpr CommandId INVALID_CMD_ID = 0;

#ifdef _DEBUG
  RenderCommand(CommandId id, bool g, const char* name)
      : cmd_id(id), global(g), cmd_name(name) {}
#else
  RenderCommand(CommandId id, bool g) : cmd_id(id), global(g) {}
#endif

  const CommandId cmd_id = INVALID_CMD_ID;
  const bool global = false;
#ifdef _DEBUG
  std::string cmd_name;
#endif
};

RENDER_COMMAND_BEGIN(CmdEableBlend, false)
RENDER_COMMAND_END

RENDER_COMMAND_BEGIN(CmdClear, false)
  std::array<float, 4> rgba;
RENDER_COMMAND_END

RENDER_COMMAND_BEGIN(CmdPresent, false)
RENDER_COMMAND_END

RENDER_COMMAND_BEGIN(CmdUpdateTexture, true)
  std::shared_ptr<const Image> image;
  std::shared_ptr<void> impl_data;
RENDER_COMMAND_END

RENDER_COMMAND_BEGIN(CmdDestoryTexture, true)
  std::shared_ptr<void> impl_data;
RENDER_COMMAND_END

RENDER_COMMAND_BEGIN(CmdActivateTexture, false)
  std::shared_ptr<void> impl_data;
RENDER_COMMAND_END

RENDER_COMMAND_BEGIN(CmdCreateGeometry, true)
  std::shared_ptr<const Mesh> mesh;
  std::shared_ptr<void> impl_data;
RENDER_COMMAND_END

RENDER_COMMAND_BEGIN(CmdDestroyGeometry, true)
  std::shared_ptr<void> impl_data;
RENDER_COMMAND_END

RENDER_COMMAND_BEGIN(CmdDrawGeometry, false)
  std::shared_ptr<void> impl_data;
RENDER_COMMAND_END

RENDER_COMMAND_BEGIN(CmdCreateShader, true)
  std::shared_ptr<const ShaderSource> source;
  VertexDescripton vertex_description;
  std::shared_ptr<void> impl_data;
RENDER_COMMAND_END

RENDER_COMMAND_BEGIN(CmdDestroyShader, true)
  std::shared_ptr<void> impl_data;
RENDER_COMMAND_END

RENDER_COMMAND_BEGIN(CmdActivateShader, false)
  std::shared_ptr<void> impl_data;
RENDER_COMMAND_END

RENDER_COMMAND_BEGIN(CmdSetUniformVec2, false)
  std::string name;
  base::Vector2 v;
  std::shared_ptr<void> impl_data;
RENDER_COMMAND_END

RENDER_COMMAND_BEGIN(CmdSetUniformVec3, false)
  std::string name;
  base::Vector3 v;
  std::shared_ptr<void> impl_data;
RENDER_COMMAND_END

RENDER_COMMAND_BEGIN(CmdSetUniformVec4, false)
  std::string name;
  base::Vector4 v;
  std::shared_ptr<void> impl_data;
RENDER_COMMAND_END

RENDER_COMMAND_BEGIN(CmdSetUniformMat4, false)
  std::string name;
  base::Matrix4x4 m;
  std::shared_ptr<void> impl_data;
RENDER_COMMAND_END

RENDER_COMMAND_BEGIN(CmdSetUniformInt, false)
  std::string name;
  int i;
  std::shared_ptr<void> impl_data;
RENDER_COMMAND_END

RENDER_COMMAND_BEGIN(CmdSetUniformFloat, false)
  std::string name;
  float f;
  std::shared_ptr<void> impl_data;
RENDER_COMMAND_END

}  // namespace eng

#endif  // RENDER_COMMAND_H
