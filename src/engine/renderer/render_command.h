#ifndef RENDER_COMMAND_H
#define RENDER_COMMAND_H

#include "../../base/hash.h"
#include "../../base/vecmath.h"
#include "types.h"
#include <memory>
#include <string>
#include <array>

namespace eng {

class Image;

// Global render commands are guaranteed to be processed. Others commands are
// frame specific and can be discared by the renderer.

#ifdef _DEBUG
#define RENDER_COMMAND_BEGIN(NAME, GLOBAL) \
  struct NAME : RenderCommand { \
    static constexpr CommandId CMD_ID = HHASH(#NAME); \
    NAME() : RenderCommand(CMD_ID, GLOBAL, #NAME) {}
#define RENDER_COMMAND_END };
#else
#define RENDER_COMMAND_BEGIN(NAME, GLOBAL) \
  struct NAME : RenderCommand { \
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
  RenderCommand(CommandId id, bool g)
      : cmd_id(id), global(g) {}
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

RENDER_COMMAND_BEGIN(CmdCreateTexture, true)
  int id;
  std::shared_ptr<const Image> image;
RENDER_COMMAND_END

RENDER_COMMAND_BEGIN(CmdDestoryTexture, true)
  int id;
RENDER_COMMAND_END

RENDER_COMMAND_BEGIN(CmdActivateTexture, false)
  int id;
RENDER_COMMAND_END

RENDER_COMMAND_BEGIN(CmdCreateGeometry, true)
  int id;
  Primitive primitive;
  std::string vertex_description;
  int num_vertices;
  const void* vertices;
  unsigned int index_description;
  int num_indices;
  const void *indices;
RENDER_COMMAND_END

RENDER_COMMAND_BEGIN(CmdDestroyGeometry, true)
  int id;
RENDER_COMMAND_END

RENDER_COMMAND_BEGIN(CmdDrawGeometry, false)
  int id;
RENDER_COMMAND_END

RENDER_COMMAND_BEGIN(CmdCreateShader, true)
  int id;
  std::unique_ptr<char[]> vertex_source;
  std::unique_ptr<char[]> fragment_source;
  std::string vertex_description;
RENDER_COMMAND_END

RENDER_COMMAND_BEGIN(CmdDestroyShader, true)
  int id;
RENDER_COMMAND_END

RENDER_COMMAND_BEGIN(CmdActivateShader, false)
  int id;
RENDER_COMMAND_END

RENDER_COMMAND_BEGIN(CmdSetUniformVec2, false)
  int id;
  std::string name;
  Vector2 v;
RENDER_COMMAND_END

RENDER_COMMAND_BEGIN(CmdSetUniformVec3, false)
  int id;
  std::string name;
  Vector3 v;
RENDER_COMMAND_END

RENDER_COMMAND_BEGIN(CmdSetUniformVec4, false)
  int id;
  std::string name;
  Vector4 v;
RENDER_COMMAND_END

RENDER_COMMAND_BEGIN(CmdSetUniformMat4, false)
  int id;
  std::string name;
  Matrix4x4 m;
RENDER_COMMAND_END

RENDER_COMMAND_BEGIN(CmdSetUniformInt, false)
  int id;
  std::string name;
  int i;
RENDER_COMMAND_END

RENDER_COMMAND_BEGIN(CmdSetUniformFloat, false)
  int id;
  std::string name;
  float f;
RENDER_COMMAND_END

} // namespace eng

#endif // RENDER_COMMAND_H
