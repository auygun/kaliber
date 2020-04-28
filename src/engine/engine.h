#ifndef ENGINE_H
#define ENGINE_H

#include <memory>
#include "../base/font.h"
#include "renderer/geometry.h"
#include "renderer/renderer.h"
#include "renderer/shader.h"
#include "text_box.h"
#include "asset_manager/asset_manager.h"
#include <list>

#if defined(__ANDROID__)
struct ANativeWindow;
#endif

namespace engine {

class Game;
class Drawable;

class Engine {
 public:
  static Engine& Get();

  bool Init();

  void Shutdown();

  void AddDrawable(Drawable* drawable);
  void RemoveDrawable(Drawable* drawable);

  void Update(float delta_time);
  void Draw(float frame_frac);

  void TrimMemory();

  Vector2 ToScale(int width, int height);
  void TransformPosition(Vector2& vec);

  AssetManager& GetAssetManager() { return asset_manager_; }
  Renderer& GetRenderer() { return renderer_; }
  Geometry& GetQuad() { return quad_; }
  Shader& GetPassThroughShader() { return pass_through_shader_; }
  Fontx& GetFont() { return font_; }

  float seconds_accumulated() { return seconds_accumulated_; }

 private:
  std::unique_ptr<Game> game_;

  AssetManager asset_manager_;

  Renderer renderer_;

  Geometry quad_;
  Shader pass_through_shader_;

  Fontx font_;

  TextBox stats_; // TODO: add to drawables.

  std::list<Drawable*> drawables_;

  float seconds_accumulated_ = 0.0f;

  bool CreateRenderResources();

  void Clear();
  void Present();
};

}  // namespace engine

#endif  // ENGINE_H
