#ifndef ENGINE_H
#define ENGINE_H

#include <memory>
#include "../base/font.h"
#include "renderer/igeometry.h"
#include "renderer/renderer.h"
#include "renderer/ishader.h"

#if defined(__ANDROID__)
struct ANativeWindow;
#endif

namespace engine {

class Game;

class Engine {
 public:
  static Engine& Get();

  bool Init();

  void Shutdown();

  void Update(float delta_time);
  void Draw(float frame_frac);

  void TrimMemory();

  Renderer& GetRenderer() { return renderer_; }
  IGeometry& GetQuad() { return quad_; }
  IShader& GetPassThroughShader() { return pass_through_shader_; }
  Fontx& GetFont() { return font_; }

 private:
  std::unique_ptr<Game> game_;

  Renderer renderer_;

  IGeometry quad_;
  IShader pass_through_shader_;

  Fontx font_;

  bool CreateRenderResources();

  void Clear();
  void Present();
};

}  // namespace engine

#endif  // ENGINE_H
