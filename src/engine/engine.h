#ifndef ENGINE_H
#define ENGINE_H

#include <deque>
#include <memory>
#include <unordered_map>

#include "../base/random_generator.h"
#include "../base/task_runner.h"
#include "../base/vecmath.h"
#include "image_quad.h"
#include "renderer/geometry.h"
#include "renderer/shader.h"

namespace eng {

class Asset;
class Font;
class Game;
class InputEvent;
class Renderer;
struct RenderCommand;
class Platform;

namespace internal {

class AssetFactoryBase {
 public:
  AssetFactoryBase(const std::string& name) : name_(name) {}
  virtual ~AssetFactoryBase() = default;

  virtual std::shared_ptr<eng::Asset> Create() = 0;

  const std::string& name() { return name_; };

 private:
  std::string name_;
};

template <typename T>
class AssetFactory : public AssetFactoryBase {
 public:
  ~AssetFactory() override = default;

  AssetFactory(const std::string& name) : AssetFactoryBase(name) {}

  std::shared_ptr<eng::Asset> Create() override {
    return std::make_shared<T>();
  }
};

}  // namespace internal

class Engine {
 public:
  Engine();
  ~Engine();

  static Engine& Get();

  bool Init(Platform* platform);

  void Shutdown();

  void Update(float delta_time);
  void Draw(float frame_frac);

  void LostFocus();
  void GainedFocus();

  void TrimMemory();

  void Exit();

  // Return screen size in viewport scale.
  base::Vector2 GetScreenSize() const { return screen_size_; }

  // Convert size from pixels to viewport scale.
  base::Vector2 ToScale(const base::Vector2& vec);

  // Convert position form pixels to viewport coordinates.
  base::Vector2 ToPosition(const base::Vector2& vec);

  // Returns immutable asset that can be accessed between multiple threads
  // without locking. Returns nullptr if no asset was found with the given name.
  template <typename T>
  std::shared_ptr<const T> GetAsset(const std::string& name) {
    internal::AssetFactory<T> factory(name);
    return std::dynamic_pointer_cast<T>(GetAssetInternal(factory));
  }

  void AddInputEvent(std::unique_ptr<InputEvent> event);
  std::unique_ptr<InputEvent> GetNextInputEvent();

  void EnqueueRenderCommand(std::unique_ptr<RenderCommand> cmd);

  // Access to the render resources.
  Geometry& GetQuad() { return quad_; }
  Shader& GetPassThroughShader() { return pass_through_shader_; }
  Shader& GetSolidShader() { return solid_shader_; }

  std::shared_ptr<const eng::Font> GetSystemFont() { return system_font_; }

  base::RandomGenerator& GetRandomGenerator() { return random_; }

  Game* GetGame() { return game_.get(); }

  // Return screen width/height in pixels.
  int GetScreenWidth() const;
  int GetScreenHeight() const;

  const base::Matrix4x4& GetProjectionMarix() const;

  int GetDeviceDpi() const;

  const std::string& GetRootPath() const;

  bool IsMobile() const;

  float seconds_accumulated() const { return seconds_accumulated_; }

 private:
  std::unique_ptr<Game> game_;

  // Asset cache.
  std::unordered_map<std::string, std::shared_ptr<Asset>> assets_;

  Platform* platform_ = nullptr;

  Renderer* renderer_ = nullptr;

  Geometry quad_;
  Shader pass_through_shader_;
  Shader solid_shader_;

  base::Vector2 screen_size_ = {0, 0};

  std::shared_ptr<const eng::Font> system_font_;

  ImageQuad stats_;

  float fps_seconds_ = 0;
  int fps_ = 0;

  float seconds_accumulated_ = 0.0f;

  std::deque<std::unique_ptr<InputEvent>> input_queue_;

  base::TaskRunner task_runner_;

  base::RandomGenerator random_;

  std::shared_ptr<Asset> GetAssetInternal(internal::AssetFactoryBase& factory);

  void ContextLost();

  bool CreateRenderResources();

  void KillUnusedResources(float delta_time);

  void SetSatsVisible(bool visible);
  void PrintStats();

  Engine(const Engine&) = delete;
  Engine& operator=(const Engine&) = delete;
};

}  // namespace eng

#endif  // ENGINE_H
