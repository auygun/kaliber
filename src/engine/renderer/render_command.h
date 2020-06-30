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
  // frame specific and can be discared by the renderer.
  const bool global = false;

#ifdef _DEBUG
  std::string cmd_name;
#endif
};

RENDER_COMMAND_BEGIN(CmdPresent)
RENDER_COMMAND_END

RENDER_COMMAND_BEGIN(CmdUpdateTexture)
  std::unique_ptr<Image> image;
  std::shared_ptr<void> impl_data;
RENDER_COMMAND_END

RENDER_COMMAND_BEGIN(CmdDestoryTexture)
  std::shared_ptr<void> impl_data;
RENDER_COMMAND_END

RENDER_COMMAND_BEGIN(CmdActivateTexture)
  std::shared_ptr<void> impl_data;
RENDER_COMMAND_END

RENDER_COMMAND_BEGIN(CmdCreateGeometry)
  std::unique_ptr<Mesh> mesh;
  std::shared_ptr<void> impl_data;
RENDER_COMMAND_END

RENDER_COMMAND_BEGIN(CmdDestroyGeometry)
  std::shared_ptr<void> impl_data;
RENDER_COMMAND_END

RENDER_COMMAND_BEGIN(CmdDrawGeometry)
  std::shared_ptr<void> impl_data;
RENDER_COMMAND_END

RENDER_COMMAND_BEGIN(CmdCreateShader)
  std::unique_ptr<ShaderSource> source;
  VertexDescripton vertex_description;
  std::shared_ptr<void> impl_data;
RENDER_COMMAND_END

RENDER_COMMAND_BEGIN(CmdDestroyShader)
  std::shared_ptr<void> impl_data;
RENDER_COMMAND_END

RENDER_COMMAND_BEGIN(CmdActivateShader)
  std::shared_ptr<void> impl_data;
RENDER_COMMAND_END

RENDER_COMMAND_BEGIN(CmdSetUniformVec2)
  std::string name;
  base::Vector2 v;
  std::shared_ptr<void> impl_data;
RENDER_COMMAND_END

RENDER_COMMAND_BEGIN(CmdSetUniformVec3)
  std::string name;
  base::Vector3 v;
  std::shared_ptr<void> impl_data;
RENDER_COMMAND_END

RENDER_COMMAND_BEGIN(CmdSetUniformVec4)
  std::string name;
  base::Vector4 v;
  std::shared_ptr<void> impl_data;
RENDER_COMMAND_END

RENDER_COMMAND_BEGIN(CmdSetUniformMat4)
  std::string name;
  base::Matrix4x4 m;
  std::shared_ptr<void> impl_data;
RENDER_COMMAND_END

RENDER_COMMAND_BEGIN(CmdSetUniformInt)
  std::string name;
  int i;
  std::shared_ptr<void> impl_data;
RENDER_COMMAND_END

RENDER_COMMAND_BEGIN(CmdSetUniformFloat)
  std::string name;
  float f;
  std::shared_ptr<void> impl_data;
RENDER_COMMAND_END

}  // namespace eng

#endif  // RENDER_COMMAND_H
