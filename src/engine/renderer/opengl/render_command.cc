#include "render_command.h"

#include "../../image.h"
#include "../../mesh.h"
#include "../../shader_source.h"

#ifdef _DEBUG
#define RENDER_COMMAND_IMPL(NAME, GLOBAL) \
  NAME::NAME() : RenderCommand(CMD_ID, GLOBAL, #NAME) {}
#else
#define RENDER_COMMAND_IMPL(NAME, GLOBAL) \
  NAME::NAME() : RenderCommand(CMD_ID, GLOBAL) {}
#endif

namespace eng {

RENDER_COMMAND_IMPL(CmdPresent, false);
RENDER_COMMAND_IMPL(CmdInvalidateAllResources, true);
RENDER_COMMAND_IMPL(CmdCreateTexture, true);
RENDER_COMMAND_IMPL(CmdUpdateTexture, true);
RENDER_COMMAND_IMPL(CmdDestoryTexture, true);
RENDER_COMMAND_IMPL(CmdActivateTexture, false);
RENDER_COMMAND_IMPL(CmdCreateGeometry, true);
RENDER_COMMAND_IMPL(CmdDestroyGeometry, true);
RENDER_COMMAND_IMPL(CmdDrawGeometry, false);
RENDER_COMMAND_IMPL(CmdCreateShader, true);
RENDER_COMMAND_IMPL(CmdDestroyShader, true);
RENDER_COMMAND_IMPL(CmdActivateShader, false);
RENDER_COMMAND_IMPL(CmdSetUniformVec2, false);
RENDER_COMMAND_IMPL(CmdSetUniformVec3, false);
RENDER_COMMAND_IMPL(CmdSetUniformVec4, false);
RENDER_COMMAND_IMPL(CmdSetUniformMat4, false);
RENDER_COMMAND_IMPL(CmdSetUniformInt, false);
RENDER_COMMAND_IMPL(CmdSetUniformFloat, false);

}  // namespace eng
