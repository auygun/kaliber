#ifndef RENDERER_H
#define RENDERER_H

#include "opengl.h"
#if defined(__linux__) && !defined(__ANDROID__)
#include <X11/Xlib.h>
#include "../../third_party/glew/glxew.h"
#endif

#include <set>
#include <string>

#if defined(__ANDROID__)
struct ANativeWindow;
#endif

namespace engine {

class Renderer {
public:
  Renderer();

#if defined(__ANDROID__)
  bool Init(ANativeWindow* window);
#endif

  bool Init(int width, int height);
  void Shutdown();

  void EnableAlphaBlending();
  void Clear(const float *rgba);
  void Present();

  bool SupportsETC1() const     { return textureCompression.etc1; }
  bool SupportsDXT1() const     { return textureCompression.dxt1 || textureCompression.s3tc; }
  bool SupportsDXT5() const     { return textureCompression.s3tc; }
  bool SupportsATC() const      { return textureCompression.atc; }

  bool SupportsVAO() const      { return vertexArrayObjects; }
  bool SupportsKHRImage() const { return khrImage && eglImage; }

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

  TextureCompression  textureCompression;
  bool                vertexArrayObjects,
                      khrImage,
                      eglImage;

#if defined(__linux__) && !defined(__ANDROID__)
  Display* display = NULL;
  Window window = 0;
  GLXContext glxContext = NULL;
#endif

  void PlatformInit(const std::set<std::string> &extensions);

  bool SetupOpenGLWindow(int width, int height);
  void ShutdownOpenGLWindow();
  void UpdateOpenGLWindow();
};

} // namespace engine

#endif // RENDERER_H
