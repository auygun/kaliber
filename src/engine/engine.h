#ifndef ENGINE_H
#define ENGINE_H

#include "../base/timer.h"
#include "renderer/renderer.h"
#include "font.h"

#if defined(__ANDROID__)
struct ANativeWindow;
#endif

namespace engine {

struct EngineConfig;

class Engine {
public:
  static Engine &Get();

#if defined(__ANDROID__)
  bool Init(ANativeWindow* window);
#else
  bool Init(const EngineConfig &config);
#endif

  void Shutdown();
  
  void Update();
  void Clear();
  void Present();

  Timer &GetTimer()         { return timer; }
  Renderer &GetRenderer()   { return renderer; }
  Font &GetFont()           { return font; }

  static int Run();

private:
  Timer           timer;
  Renderer        renderer;
  Font            font;
};

} // namespace engine

#endif // ENGINE_H
