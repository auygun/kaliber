#ifndef ENGINE_ENGINE_H
#define ENGINE_ENGINE_H

#include <deque>
#include <functional>
#include <list>
#include <memory>
#include <unordered_map>

#include "base/random.h"
#include "base/thread_pool.h"
#include "base/timer.h"
#include "engine/asset_manager.h"
#include "engine/audio/audio_mixer.h"
#include "engine/imgui_backend.h"
#include "engine/input_system.h"
#include "engine/platform/platform_observer.h"
#include "engine/system.h"
#include "engine/world.h"
#include "engine/renderer/render_graph.h"

class TextureCompressor;

namespace eng {

class AudioMixer;
class Game;
class Platform;
class Renderer;
enum class RendererType;

class Engine : public PlatformObserver {
 public:
  Engine(Platform* platform);
  ~Engine() noexcept override;

  static Engine& Get();

  void Run();

  void CreateRenderer(RendererType type);
  RendererType GetRendererType();

  void Exit();

  // Convert size from pixels to viewport scale.
  base::Vector2f ToViewportScale(const base::Vector2f& vec);

  // Convert position form pixels to viewport coordinates.
  base::Vector2f ToViewportPosition(const base::Vector2f& vec);

  // Vibrate (if supported by the platform) for the specified duration.
  void Vibrate(int duration);

  void ShowInterstitialAd();

  void ShareFile(const std::string& file_name);

  void SetKeepScreenOn(bool keep_screen_on);

  void SetEnableAudio(bool enable);

  void SetEnableVibration(bool enable) { vibration_enabled_ = enable; }

  Platform* GetPlatform() { return platform_; }

  Renderer* GetRenderer() { return renderer_.get(); }

  AssetManager& GetAssetManager() { return asset_manager_; }

  AudioMixer& GetAudioMixer() { return audio_mixer_; }

  base::Randomf& GetRandomGenerator() { return random_; }

  TextureCompressor* GetTextureCompressor(bool opacity);

  Game* GetGame() { return game_.get(); }

  World& GetWorld() { return world_; }

  // Return screen width/height in pixels.
  int GetScreenWidth() const;
  int GetScreenHeight() const;

  // Return screen size in viewport scale.
  base::Vector2f GetViewportSize() const { return screen_size_; }

  const std::string& GetRootPath() const;

  const std::string& GetDataPath() const;

  const std::string& GetSharedDataPath() const;

  size_t GetAudioHardwareSampleRate();

  bool IsMobile() const;

  float seconds_accumulated() const { return seconds_accumulated_; }

  float time_step() { return time_step_; }

  int fps() const { return fps_; }

 private:
  static Engine* singleton;

  Platform* platform_ = nullptr;

  base::Vector2f screen_size_ = {0, 0};

  bool stats_visible_ = false;

  float fps_seconds_ = 0;
  int fps_ = 0;

  base::DeltaTimer timer_;
  float seconds_accumulated_ = 0.0f;
  float time_step_ = 1.0f / 60.0f;
  size_t tick_ = 0;

  bool vibration_enabled_ = true;

  std::unique_ptr<TextureCompressor> tex_comp_opaque_;
  std::unique_ptr<TextureCompressor> tex_comp_alpha_;

  std::unique_ptr<Renderer> renderer_;

  AssetManager asset_manager_;

  AudioMixer audio_mixer_;

  InputSystem input_system_;

  std::vector<std::unique_ptr<System>> systems_;

  base::Randomf random_;

  ImguiBackend imgui_backend_;

  World world_;

  RenderGraph render_graph_;

  std::unique_ptr<Game> game_;

  base::ThreadPool thread_pool_;

  void Initialize();

  void FixedUpdate(float delta_time);
  void Update(float delta_time);
  void Draw(float frame_frac);

  // PlatformObserver implementation
  void OnWindowCreated() final;
  void OnWindowDestroyed() final;
  void OnWindowResized(int width, int height) final;
  void LostFocus() final;
  void GainedFocus(bool from_interstitial_ad) final;

  void CreateRendererInternal(RendererType type);

  void CreateTextureCompressors();

  void ContextLost();

  void ShowStats();

  // This tracks the selected node in the UI
  Entity selected_entity_{NULL_ENTITY};

  // Used for deferred reparenting
  Entity dragged_entity_{NULL_ENTITY};
  Entity new_parent_entity_{NULL_ENTITY};

  void DrawSceneGraphUI();

  void DrawSceneNodeIterative(Entity entity);

  void DrawNodeInspector();

  Engine(const Engine&) = delete;
  Engine& operator=(const Engine&) = delete;
};

}  // namespace eng

#endif  // ENGINE_ENGINE_H
