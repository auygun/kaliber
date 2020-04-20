#ifndef ENGINE_H
#define ENGINE_H

#include "../base/font.h"
#include "renderer/renderer.h"
#include "renderer/geometry.h"
#include "renderer/shader.h"
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

  Renderer &GetRenderer()   { return renderer_; }
  Geometry& GetQuad() { return quad_; }
  Shader& GetPassThroughShader() { return pass_through_shader_; }
  Fontx &GetFont()           { return font_; }

private:
  std::unique_ptr<Game> game_;

  Renderer renderer_;

  Geometry quad_;
  Shader pass_through_shader_;

  Fontx font_;

  bool CreateRenderResources();

  void Clear();
  void Present();
};

} // namespace engine

#endif // ENGINE_H
