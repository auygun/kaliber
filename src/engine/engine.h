#ifndef ENGINE_H
#define ENGINE_H

#include <memory>
#include "../base/font.h"
#include "../base/vecmath.h"
#include "renderer/geometry.h"
#include "renderer/renderer.h"
#include "renderer/shader.h"
#include "image_quad.h"
#include "asset_manager/asset_manager.h"
#include <list>
#include <deque>
#include <unordered_map>


namespace eng {

class Game;
class Drawable;
class InputEvent;

class Engine : public Renderer::Delegate {
 public:
  static Engine& Get();

  bool Init();

  void Shutdown();

  void AddDrawable(Drawable* drawable);
  void RemoveDrawable(Drawable* drawable);

  void Update(float delta_time);
  void Draw(float frame_frac);

  void TrimMemory();

  Vector2 GetScreenSize();

  Vector2 ToScale(const Vector2& vec);
  Vector2 ToPosition(const Vector2& vec);

  int AcquireTextureResource(std::shared_ptr<const Image> image);
  void ReturnTextureResource(int resource_id);

  void AddInputEvent(std::unique_ptr<InputEvent> event);
  std::unique_ptr<InputEvent> GetNextInputEvent();

  AssetManager& GetAssetManager() { return asset_manager_; }
  Renderer& GetRenderer() { return renderer_; }
  Geometry& GetQuad() { return quad_; }
  Shader& GetPassThroughShader() { return pass_through_shader_; }
  Fontx& GetFont() { return font_; }

  Game* GetGame() { return game_.get(); }

  int GetScreenWidth() const { return renderer_.screen_width(); }
  int GetScreenHeight() const { return renderer_.screen_height(); }

  const  Matrix4x4& GetProjectionMarix() const {
    return renderer_.projection();
  }

  float seconds_accumulated() const { return seconds_accumulated_; }

 private:
  struct TextureResource {
    int resource_id = 0;
    int ref_count = 0;
    float time_to_die_ = 0.0f;
  };

  std::unique_ptr<Game> game_;

  std::unordered_map<std::string, TextureResource> texture_resources_;
  // TODO: Recycle resource ids.
  int last_texture_resource_id_ = 0;

  AssetManager asset_manager_;

  Renderer renderer_;

  Geometry quad_;
  Shader pass_through_shader_;

  Fontx font_;

  ImageQuad stats_;

  std::list<Drawable*> drawables_;

  float seconds_accumulated_ = 0.0f;

  // TODO: Move to InputQueue class.
  std::deque<std::unique_ptr<InputEvent>> input_queue_;

  void ContextLost() override;

  bool CreateRenderResources();

  void KillUnusedResources(float delta_time);

  void Clear();
  void Present();

  void ShowStats(bool show);
  void PrintStats();
};

}  // namespace eng

#endif  // ENGINE_H
