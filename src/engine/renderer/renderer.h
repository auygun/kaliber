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

  bool SupportsETC1() const     { return textureCompression.etc1; }
  bool SupportsDXT1() const     { return textureCompression.dxt1 || textureCompression.s3tc; }
  bool SupportsDXT5() const     { return textureCompression.s3tc; }
  bool SupportsATC() const      { return textureCompression.atc; }

  bool SupportsVAO() const      { return vertexArrayObjects; }
  bool SupportsKHRImage() const { return khrImage && eglImage; }

#if defined(__ANDROID__)
#endif

private:
  struct TextureCompression {
    unsigned  etc1    : 1,
              dxt1    : 1,
              latc    : 1,
              s3tc    : 1,
              pvrtc   : 1,
              atc     : 1;

    TextureCompression()
      : etc1(false)
      , dxt1(false)
      , latc(false)
      , s3tc(false)
      , pvrtc(false)
      , atc(false) {
    }
  };

  TextureCompression textureCompression;
  bool vertexArrayObjects = false;
  bool khrImage = false;
  bool eglImage = false;

  int screen_width_ = 0;
  int screen_height_ = 0;

#if defined(__linux__) && !defined(__ANDROID__)
  Display* display = NULL;
  Window window = 0;
  GLXContext glxContext = NULL;
#endif

  std::set<std::string> SetupExtensions();

  void LogVersion();
};

} // namespace engine

#endif // RENDERER_H
