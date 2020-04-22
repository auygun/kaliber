#include "renderer.h"
#include <sstream>
#include "../../base/log.h"

namespace engine {

void Renderer::EnqueueCommand(std::unique_ptr<RenderCommand> cmd) {
  command_queue_.push_back(std::move(cmd));
}

void Renderer::WorkerMain() {
  for(;;) {
    if (command_queue_.empty())
      return;
    std::unique_ptr<RenderCommand> cmd;
    cmd.swap(command_queue_.front());
    command_queue_.pop_front();

    switch(cmd->cmd_id) {
    case HASH("CmdEableBlend"):
      HandleCmdEnableBlend(std::move(cmd));
      break;
    case HASH("CmdClear"):
      HandleCmdClear(std::move(cmd));
      break;
    case HASH("CmdPresent"):
      HandleCmdPresent(std::move(cmd));
      break;
    case HASH("CmdCreateTexture"):
      HandleCmdCreateTexture(std::move(cmd));
      break;
    case HASH("CmdDestoryTexture"):
      HandleCmdDestoryTexture(std::move(cmd));
      break;
    case HASH("CmdActivateTexture"):
      HandleCmdActivateTexture(std::move(cmd));
      break;
    case HASH("CmdCreateGeometry"):
      HandleCmdCreateGeometry(std::move(cmd));
      break;
    case HASH("CmdDestroyGeometry"):
      HandleCmdDestroyGeometry(std::move(cmd));
      break;
    case HASH("CmdDrawGeometry"):
      HandleCmdDrawGeometry(std::move(cmd));
      break;
    case HASH("CmdCreateShader"):
      HandleCmdCreateShader(std::move(cmd));
      break;
    case HASH("CmdDestroyShader"):
      HandleCmdDestroyShader(std::move(cmd));
      break;
    case HASH("CmdActivateShader"):
      HandleCmdActivateShader(std::move(cmd));
      break;
    case HASH("CmdSetUniformVec2"):
      HandleCmdSetUniformVec2(std::move(cmd));
      break;
    case HASH("CmdSetUniformVec3"):
      HandleCmdSetUniformVec3(std::move(cmd));
      break;
    case HASH("CmdSetUniformInt"):
      HandleCmdSetUniformInt(std::move(cmd));
      break;
    default:
      // assert(false);
      break;
    }
  }
}

void Renderer::HandleCmdEnableBlend(std::unique_ptr<RenderCommand> cmd) {
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void Renderer::HandleCmdClear(std::unique_ptr<RenderCommand> cmd) {
  auto *c = static_cast<CmdClear*>(cmd.get());
  glClearColor(c->rgba[0], c->rgba[1], c->rgba[2], c->rgba[3]);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Renderer::HandleCmdCreateTexture(std::unique_ptr<RenderCommand> cmd) {
  auto *c = static_cast<CmdCreateTexture*>(cmd.get());
  auto t = std::make_unique<Texture>();
  t->Create(*(c->image.get()));
  texture_map_[c->id] = std::move(t);
}

void Renderer::HandleCmdDestoryTexture(std::unique_ptr<RenderCommand> cmd) {
  auto *c = static_cast<CmdDestoryTexture*>(cmd.get());
  texture_map_[c->id]->Destroy();
  texture_map_.erase(c->id);
}

void Renderer::HandleCmdActivateTexture(std::unique_ptr<RenderCommand> cmd) {
  auto *c = static_cast<CmdActivateTexture*>(cmd.get());
  texture_map_[c->id]->Activate();
}

void Renderer::HandleCmdCreateGeometry(std::unique_ptr<RenderCommand> cmd) {
  auto *c = static_cast<CmdCreateGeometry*>(cmd.get());
  auto g = std::make_unique<Geometry>();
  g->Create(c->primitive, c->vertex_description, c->num_vertices, c->vertices,
      c->index_description, c->num_indices, c->indices);
  geometry_map_[c->id] = std::move(g);
}

void Renderer::HandleCmdDestroyGeometry(std::unique_ptr<RenderCommand> cmd) {
  auto *c = static_cast<CmdDestroyGeometry*>(cmd.get());
  geometry_map_[c->id]->Destroy();
  geometry_map_.erase(c->id);
}

void Renderer::HandleCmdDrawGeometry(std::unique_ptr<RenderCommand> cmd) {
  auto *c = static_cast<CmdDrawGeometry*>(cmd.get());
  geometry_map_[c->id]->Draw();
}

void Renderer::HandleCmdCreateShader(std::unique_ptr<RenderCommand> cmd) {
  auto *c = static_cast<CmdCreateShader*>(cmd.get());
  auto s = std::make_unique<Shader>();
  s->Create(c->name, c->vertex_description);
  shader_map_[c->id] = std::move(s);
}

void Renderer::HandleCmdDestroyShader(std::unique_ptr<RenderCommand> cmd) {
  auto *c = static_cast<CmdDestroyShader*>(cmd.get());
  shader_map_[c->id]->Destroy();
  shader_map_.erase(c->id);
}

void Renderer::HandleCmdActivateShader(std::unique_ptr<RenderCommand> cmd) {
  auto *c = static_cast<CmdActivateShader*>(cmd.get());
  shader_map_[c->id]->Activate();
}

void Renderer::HandleCmdSetUniformVec2(std::unique_ptr<RenderCommand> cmd) {
  auto *c = static_cast<CmdSetUniformVec2*>(cmd.get());
  shader_map_[c->id]->SetUniform(c->name, c->v);
}

void Renderer::HandleCmdSetUniformVec3(std::unique_ptr<RenderCommand> cmd) {
  auto *c = static_cast<CmdSetUniformVec3*>(cmd.get());
  shader_map_[c->id]->SetUniform(c->name, c->v);
}

void Renderer::HandleCmdSetUniformInt(std::unique_ptr<RenderCommand> cmd) {
  auto *c = static_cast<CmdSetUniformInt*>(cmd.get());
  shader_map_[c->id]->SetUniform(c->name, c->i);
}

std::unordered_set<std::string> Renderer::SetupExtensions() {
  std::stringstream stream((const char*)glGetString(GL_EXTENSIONS));
  std::string token;
  std::unordered_set<std::string> extensions;
  while (std::getline(stream, token, ' '))
    extensions.insert(token);

#if 0
  LOG("  extensions:");
  for (auto& ext : extensions)
    LOG("    %s\n", ext.c_str());
#endif

  // Check for supported texture compression extensions.
  if (extensions.find("GL_OES_compressed_ETC1_RGB8_texture") !=
      extensions.end())
    texture_compression_.etc1 = true;
  if (extensions.find("GL_EXT_texture_compression_dxt1") != extensions.end())
    texture_compression_.dxt1 = true;
  if (extensions.find("GL_EXT_texture_compression_latc") != extensions.end())
    texture_compression_.latc = true;
  if (extensions.find("GL_EXT_texture_compression_s3tc") != extensions.end())
    texture_compression_.s3tc = true;
  if (extensions.find("GL_IMG_texture_compression_pvrtc") != extensions.end())
    texture_compression_.pvrtc = true;
  if (extensions.find("GL_AMD_compressed_ATC_texture") != extensions.end() ||
      extensions.find("GL_ATI_texture_compression_atitc") != extensions.end())
    texture_compression_.atc = true;

  return extensions;
}

void Renderer::EnableBlend() {
  auto cmd = std::make_unique<CmdEableBlend>();
  EnqueueCommand(std::move(cmd));
}

void Renderer::Clear(const std::array<float, 4>& rgba) {
  auto cmd = std::make_unique<CmdClear>();
  cmd->rgba = rgba;
  EnqueueCommand(std::move(cmd));
}

void Renderer::Present() {
  auto cmd = std::make_unique<CmdPresent>();
  EnqueueCommand(std::move(cmd));
  WorkerMain();
}

void Renderer::ContextLost() {}

void Renderer::LogVersion() {
  LOG("OpenGL:\n");
  LOG("  vendor:         %s\n", (const char*)glGetString(GL_VENDOR));
  LOG("  renderer:       %s\n", (const char*)glGetString(GL_RENDERER));
  LOG("  version:        %s\n", (const char*)glGetString(GL_VERSION));
  LOG("  shader version: %s\n",
      (const char*)glGetString(GL_SHADING_LANGUAGE_VERSION));
}

}  // namespace engine
