#ifndef ENGINE_H
#define ENGINE_H

#include "renderer/renderer.h"
#include "font.h"
#include <memory>

#if defined(__ANDROID__)
struct ANativeWindow;
#endif

namespace engine {

class Game;

class Engine {
public:
  static Engine &Get();

  bool Init();

  void Shutdown();
  
  void Update(float delta_time);
  void Draw(float frame_frac);

  void TrimMemory();

  Renderer &GetRenderer()   { return renderer; }
  Font &GetFont()           { return font; }

private:
  std::unique_ptr<Game> game_;

  Renderer        renderer;
  Font            font;

  void Clear();
  void Present();
};

} // namespace engine

#endif // ENGINE_H
