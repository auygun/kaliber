#ifndef ENGINE_H
#define ENGINE_H

#include <deque>
#include <memory>
#include <unordered_map>

#include "../base/random.h"
#include "../base/vecmath.h"
#include "audio/audio_forward.h"
#include "image_quad.h"
#include "renderer/render_resource.h"

class TextureCompressor;

namespace eng {

class AudioResource;
class Font;
class Game;
class InputEvent;
class Renderer;
struct RenderCommand;
class Platform;
class Geometry;
class Shader;

class Engine {
 public:
  Engine(Platform* platform, Renderer* renderer, Audio* audio);
  ~Engine();

  static Engine& Get();

  bool Initialize();

  void Shutdown();

  void Update(float delta_time);
  void Draw(float frame_frac);

  void LostFocus();
  void GainedFocus();

  void Exit();

  // Convert size from pixels to viewport scale.
  base::Vector2 ToScale(const base::Vector2& vec);

  // Convert position form pixels to viewport coordinates.
  base::Vector2 ToPosition(const base::Vector2& vec);

  template <typename T>
  std::shared_ptr<T> CreateRenderResource() {
    RenderResourceFactory<T> factory;
    return std::dynamic_pointer_cast<T>(CreateRenderResourceInternal(factory));
  }

  std::shared_ptr<AudioResource> CreateAudioResource();

  void AddInputEvent(std::unique_ptr<InputEvent> event);
  std::unique_ptr<InputEvent> GetNextInputEvent();

  // Access to the render resources.
  std::shared_ptr<Geometry> GetQuad() { return quad_; }
  std::shared_ptr<Shader> GetPassThroughShader() {
    return pass_through_shader_;
  }
  std::shared_ptr<Shader> GetSolidShader() { return solid_shader_; }

  const Font* GetSystemFont() { return system_font_.get(); }

  base::Random& GetRandomGenerator() { return random_; }

  TextureCompressor* GetTextureCompressor(bool opacity);

  Game* GetGame() { return game_.get(); }

  // Return screen width/height in pixels.
  int GetScreenWidth() const;
  int GetScreenHeight() const;

  // Return screen size in viewport scale.
  base::Vector2 GetScreenSize() const { return screen_size_; }

  const base::Matrix4x4& GetProjectionMarix() const { return projection_; }

  int GetDeviceDpi() const;

  const std::string& GetRootPath() const;

  size_t GetAudioSampleRate();

  bool IsMobile() const;

  float seconds_accumulated() const { return seconds_accumulated_; }

 private:
  static Engine* singleton;

  Platform* platform_ = nullptr;

  Renderer* renderer_ = nullptr;

  Audio* audio_;

  std::unique_ptr<Game> game_;

  std::shared_ptr<Geometry> quad_;
  std::shared_ptr<Shader> pass_through_shader_;
  std::shared_ptr<Shader> solid_shader_;

  base::Vector2 screen_size_ = {0, 0};
  base::Matrix4x4 projection_;

  std::unique_ptr<Font> system_font_;

  std::unique_ptr<TextureCompressor> tex_comp_opaque_;
  std::unique_ptr<TextureCompressor> tex_comp_alpha_;

  ImageQuad stats_;

  float fps_seconds_ = 0;
  int fps_ = 0;

  float seconds_accumulated_ = 0.0f;

  std::deque<std::unique_ptr<InputEvent>> input_queue_;

  base::Random random_;

  std::shared_ptr<RenderResource> CreateRenderResourceInternal(
      RenderResourceFactoryBase& factory);

  void ContextLost();

  bool CreateRenderResources();

  void SetSatsVisible(bool visible);
  void PrintStats();

  Engine(const Engine&) = delete;
  Engine& operator=(const Engine&) = delete;
};

}  // namespace eng

#endif  // ENGINE_H
