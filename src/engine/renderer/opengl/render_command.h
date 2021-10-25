#ifndef RENDER_COMMAND_H
#define RENDER_COMMAND_H

#include <array>
#include <memory>
#include <string>

#include "../../../base/hash.h"
#include "../../../base/vecmath.h"
#include "../renderer_types.h"

namespace eng {

class Image;
class ShaderSource;
class Mesh;

#define RENDER_COMMAND_BEGIN(NAME)                    \
  struct NAME : RenderCommand {                       \
    static constexpr CommandId CMD_ID = HHASH(#NAME); \
    NAME();
#define RENDER_COMMAND_END \
  }                        \
  ;

struct RenderCommand {
  using CommandId = size_t;
  static constexpr CommandId INVALID_CMD_ID = 0;

#ifdef _DEBUG
  RenderCommand(CommandId id, bool g, const char* name)
      : cmd_id(id), global(g), cmd_name(name) {}
#else
  RenderCommand(CommandId id, bool g) : cmd_id(id), global(g) {}
#endif

  virtual ~RenderCommand() = default;

  const CommandId cmd_id = INVALID_CMD_ID;

  // Global render commands are guaranteed to be processed. Others commands are
  // frame specific and can be discared by the renderer if not throttled.
  const bool global = false;

#ifdef _DEBUG
  std::string cmd_name;
#endif
};

RENDER_COMMAND_BEGIN(CmdPresent)
RENDER_COMMAND_END

RENDER_COMMAND_BEGIN(CmdInvalidateAllResources)
RENDER_COMMAND_END

RENDER_COMMAND_BEGIN(CmdCreateTexture)
  uint64_t resource_id;
RENDER_COMMAND_END

RENDER_COMMAND_BEGIN(CmdUpdateTexture)
  std::unique_ptr<Image> image;
  uint64_t resource_id;
RENDER_COMMAND_END

RENDER_COMMAND_BEGIN(CmdDestoryTexture)
  uint64_t resource_id;
RENDER_COMMAND_END

RENDER_COMMAND_BEGIN(CmdActivateTexture)
  uint64_t resource_id;
RENDER_COMMAND_END

RENDER_COMMAND_BEGIN(CmdCreateGeometry)
  std::unique_ptr<Mesh> mesh;
  uint64_t resource_id;
RENDER_COMMAND_END

RENDER_COMMAND_BEGIN(CmdDestroyGeometry)
  uint64_t resource_id;
RENDER_COMMAND_END

RENDER_COMMAND_BEGIN(CmdDrawGeometry)
  uint64_t resource_id;
RENDER_COMMAND_END

RENDER_COMMAND_BEGIN(CmdCreateShader)
  std::unique_ptr<ShaderSource> source;
  VertexDescripton vertex_description;
  uint64_t resource_id;
  bool enable_depth_test;
RENDER_COMMAND_END

RENDER_COMMAND_BEGIN(CmdDestroyShader)
  uint64_t resource_id;
RENDER_COMMAND_END

RENDER_COMMAND_BEGIN(CmdActivateShader)
  uint64_t resource_id;
RENDER_COMMAND_END

RENDER_COMMAND_BEGIN(CmdSetUniformVec2)
  std::string name;
  base::Vector2f v;
  uint64_t resource_id;
RENDER_COMMAND_END

RENDER_COMMAND_BEGIN(CmdSetUniformVec3)
  std::string name;
  base::Vector3f v;
  uint64_t resource_id;
RENDER_COMMAND_END

RENDER_COMMAND_BEGIN(CmdSetUniformVec4)
  std::string name;
  base::Vector4f v;
  uint64_t resource_id;
RENDER_COMMAND_END

RENDER_COMMAND_BEGIN(CmdSetUniformMat4)
  std::string name;
  base::Matrix4f m;
  uint64_t resource_id;
RENDER_COMMAND_END

RENDER_COMMAND_BEGIN(CmdSetUniformInt)
  std::string name;
  int i;
  uint64_t resource_id;
RENDER_COMMAND_END

RENDER_COMMAND_BEGIN(CmdSetUniformFloat)
  std::string name;
  float f;
  uint64_t resource_id;
RENDER_COMMAND_END

}  // namespace eng

#endif  // RENDER_COMMAND_H
