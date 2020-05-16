#ifndef ENGINE_H
#define ENGINE_H

#include <memory>
#include "../base/vecmath.h"
#include "renderer/geometry.h"
#include "renderer/shader.h"
#include "image_quad.h"
#include <deque>
#include <unordered_map>

namespace eng {

class Image;
class Font;
class Game;
class InputEvent;
class Renderer;
struct RenderCommand;
class Platform;

class Engine {
 public:
  static Engine& Get();

  bool Init(Platform* platform);

  void Shutdown();

  void Update(float delta_time);
  void Draw(float frame_frac);

  void TrimMemory();

  Vector2 GetScreenSize();

  Vector2 ToScale(const Vector2& vec);
  Vector2 ToPosition(const Vector2& vec);

  // Returns immutable image that can be accessed between multiple threads
  // without locking.
  std::shared_ptr<const Image> GetImageAsset(const std::string& name);
  std::shared_ptr<Font> GetFontAsset(const std::string& name);

  int AcquireTextureResource(std::shared_ptr<const Image> image);
  void ReturnTextureResource(int resource_id);

  void AddInputEvent(std::unique_ptr<InputEvent> event);
  std::unique_ptr<InputEvent> GetNextInputEvent();

  void EnqueueRenderCommand(std::unique_ptr<RenderCommand> cmd);

  Geometry& GetQuad() { return quad_; }
  Shader& GetPassThroughShader() { return pass_through_shader_; }
  Shader& GetSolidShader() { return solid_shader_; }

  std::shared_ptr<eng::Font> GetSystemFont() { return system_font_; }

  Game* GetGame() { return game_.get(); }

  int GetScreenWidth() const;
  int GetScreenHeight() const;

  const  Matrix4x4& GetProjectionMarix() const;

  int GetDeviceDpi() const;

  const std::string& GetRootPath() const;

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

  std::unordered_map<std::string, std::shared_ptr<const Image>> image_assets_;
  std::unordered_map<std::string, std::shared_ptr<Font>> font_assets_;

  Platform* platform_ = nullptr;

  Renderer* renderer_ = nullptr;

  Geometry quad_;
  Shader pass_through_shader_;
  Shader solid_shader_;

  std::shared_ptr<eng::Font> system_font_;

  ImageQuad stats_;

  float seconds_accumulated_ = 0.0f;

  // TODO: Move to InputQueue class.
  std::deque<std::unique_ptr<InputEvent>> input_queue_;

  void ContextLost();

  bool CreateRenderResources();

  void KillUnusedResources(float delta_time);

  void Clear();
  void Present();

  void ShowStats(bool show);
  void PrintStats();
};

}  // namespace eng

#endif  // ENGINE_H
