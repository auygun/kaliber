#include "renderer.h"
#include <sstream>
#include "../../base/log.h"

namespace engine {

std::set<std::string> Renderer::SetupExtensions() {
  std::stringstream stream((const char*)glGetString(GL_EXTENSIONS));
  std::string token;
  std::set<std::string> extensions;
  while (std::getline(stream, token, ' '))
    extensions.insert(token);
  // LOG("  extensions:");
  // for (std::set<std::string>::iterator i = extensions.begin(); i !=
  // extensions.end(); ++i)
  //   LOG("    %s\n", i->c_str());

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
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void Renderer::Clear(const float* rgba) {
  glClearColor(rgba[0], rgba[1], rgba[2], rgba[3]);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
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
