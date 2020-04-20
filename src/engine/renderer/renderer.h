#ifndef RENDERER_H
#define RENDERER_H

#include "opengl.h"
#if defined(__linux__) && !defined(__ANDROID__)
#include <X11/Xlib.h>
#include "../../third_party/glew/glxew.h"
#endif

#include <set>
#include <string>

namespace engine {

class Renderer {
public:
  Renderer() = default;

  bool Init();
  void Shutdown();

  void EnableBlend();
  void Clear(const float *rgba);
  void Present();

  void ContextLost();

  void TrimMemory();

  int GetScreenWidth() { return screen_width_; }
  int GetScreenHeight() { return screen_height_; }

  bool SupportsETC1() const     { return texture_compression_.etc1; }
  bool SupportsDXT1() const     { return texture_compression_.dxt1 || texture_compression_.s3tc; }
  bool SupportsDXT5() const     { return texture_compression_.s3tc; }
  bool SupportsATC() const      { return texture_compression_.atc; }

  bool SupportsVAO() const      { return vertex_array_objects_; }

#if defined(__linux__) && !defined(__ANDROID__)
  bool ShouldExit();
#endif

private:
  struct TextureCompression {
    unsigned  etc1    : 1;
    unsigned  dxt1    : 1;
    unsigned  latc    : 1;
    unsigned  s3tc    : 1;
    unsigned  pvrtc   : 1;
    unsigned  atc     : 1;

    TextureCompression()
      : etc1(false)
      , dxt1(false)
      , latc(false)
      , s3tc(false)
      , pvrtc(false)
      , atc(false) {
    }
  };

  TextureCompression texture_compression_;
  bool vertex_array_objects_ = false;

  int screen_width_ = 0;
  int screen_height_ = 0;

#if defined(__linux__) && !defined(__ANDROID__)
  Display* display_ = NULL;
  Window window_ = 0;
  GLXContext glx_context_ = NULL;
#endif

  std::set<std::string> SetupExtensions();

  void LogVersion();
};

} // namespace engine

#endif // RENDERER_H
