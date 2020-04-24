#ifndef RENDER_COMMAND_H
#define RENDER_COMMAND_H

#include "../../base/vecmath.h"
#include "../../base/image.h"
#include <memory>
#include <string>
#include <array>

class Image;

namespace engine {

template <size_t N>
constexpr inline size_t HORNER_HASH(size_t prime, const char (&str)[N], size_t Len = N-1)
{
    return (Len <= 1) ? str[0] : (prime * HORNER_HASH(prime, str, Len-1) + str[Len-1]);
}

#define HASH(x) (HORNER_HASH(31, x))

#ifdef _DEBUG
#define RENDER_COMMAND_BEGIN(NAME) \
  struct NAME : RenderCommand { \
    static constexpr CommandId CMD_ID = HASH(#NAME); \
    NAME() : RenderCommand(CMD_ID, #NAME) {}
#define RENDER_COMMAND_END };
#else
#define RENDER_COMMAND_BEGIN(NAME) \
  struct NAME : RenderCommand { \
    static constexpr CommandId CMD_ID = HASH(#NAME); \
    NAME() : RenderCommand(CMD_ID) {}
#define RENDER_COMMAND_END };
#endif

struct RenderCommand {
  using CommandId = size_t;
  static constexpr CommandId INVALID_CMD_ID = 0;

#ifdef _DEBUG
  RenderCommand(CommandId id, const char* name) : cmd_id(id), cmd_name(name) {}
#else
  RenderCommand(CommandId id) : cmd_id(id) {}
#endif

  const CommandId cmd_id = INVALID_CMD_ID;
#ifdef _DEBUG
  std::string cmd_name;
#endif
};

RENDER_COMMAND_BEGIN(CmdEableBlend)
RENDER_COMMAND_END

RENDER_COMMAND_BEGIN(CmdSync)
RENDER_COMMAND_END

RENDER_COMMAND_BEGIN(CmdClear)
  std::array<float, 4> rgba;
RENDER_COMMAND_END

RENDER_COMMAND_BEGIN(CmdPresent)
RENDER_COMMAND_END

RENDER_COMMAND_BEGIN(CmdCreateTexture)
  int id;
  std::unique_ptr<Image> image;
RENDER_COMMAND_END

RENDER_COMMAND_BEGIN(CmdDestoryTexture)
  int id;
RENDER_COMMAND_END

RENDER_COMMAND_BEGIN(CmdActivateTexture)
  int id;
RENDER_COMMAND_END

RENDER_COMMAND_BEGIN(CmdCreateGeometry)
  int id;
  unsigned int primitive;
  std::string vertex_description;
  int num_vertices;
  const void* vertices;
  unsigned int index_description;
  int num_indices;
  const void *indices;
RENDER_COMMAND_END

RENDER_COMMAND_BEGIN(CmdDestroyGeometry)
  int id;
RENDER_COMMAND_END

RENDER_COMMAND_BEGIN(CmdDrawGeometry)
  int id;
RENDER_COMMAND_END

RENDER_COMMAND_BEGIN(CmdCreateShader)
  int id;
  std::unique_ptr<char[]> vertex_source;
  std::unique_ptr<char[]> fragment_source;
  std::string vertex_description;
RENDER_COMMAND_END

RENDER_COMMAND_BEGIN(CmdDestroyShader)
  int id;
RENDER_COMMAND_END

RENDER_COMMAND_BEGIN(CmdActivateShader)
  int id;
RENDER_COMMAND_END

RENDER_COMMAND_BEGIN(CmdSetUniformVec2)
  int id;
  std::string name;
  Vector2 v;
RENDER_COMMAND_END

RENDER_COMMAND_BEGIN(CmdSetUniformVec3)
  int id;
  std::string name;
  Vector3 v;
RENDER_COMMAND_END

RENDER_COMMAND_BEGIN(CmdSetUniformInt)
  int id;
  std::string name;
  int i;
RENDER_COMMAND_END

} // namespace engine 

#endif // RENDER_COMMAND_H
