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
#include "engine/imgui_backend.h"
#include "engine/platform/platform_observer.h"

class TextureCompressor;

namespace eng {

class AudioMixer;
class Game;
class InputEvent;
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

  std::unique_ptr<InputEvent> GetNextInputEvent();

  // Vibrate (if supported by the platform) for the specified duration.
  void Vibrate(int duration);

  void ShowInterstitialAd();

  void ShareFile(const std::string& file_name);

  void SetKeepScreenOn(bool keep_screen_on);

  void SetEnableAudio(bool enable);

  void SetEnableVibration(bool enable) { vibration_enabled_ = enable; }

  Platform* GetPlatform() { return platform_; }

  Renderer* GetRenderer() { return renderer_.get(); }

  AudioMixer* GetAudioMixer() { return audio_mixer_.get(); }

  base::Randomf& GetRandomGenerator() { return random_; }

  TextureCompressor* GetTextureCompressor(bool opacity);

  Game* GetGame() { return game_.get(); }

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

  std::unique_ptr<Renderer> renderer_;
  std::unique_ptr<AudioMixer> audio_mixer_;
  std::unique_ptr<Game> game_;

  base::Vector2f screen_size_ = {0, 0};

  std::unique_ptr<TextureCompressor> tex_comp_opaque_;
  std::unique_ptr<TextureCompressor> tex_comp_alpha_;

  bool stats_visible_ = false;

  ImguiBackend imgui_backend_;

  float fps_seconds_ = 0;
  int fps_ = 0;

  base::DeltaTimer timer_;
  float seconds_accumulated_ = 0.0f;
  float time_step_ = 1.0f / 60.0f;
  size_t tick_ = 0;

  bool vibration_enabled_ = true;

  std::deque<std::unique_ptr<InputEvent>> input_queue_;

  base::ThreadPool thread_pool_;
  base::Randomf random_;

  void Initialize();

  void Update(float delta_time);
  void Draw(float frame_frac);

  // PlatformObserver implementation
  void OnWindowCreated() final;
  void OnWindowDestroyed() final;
  void OnWindowResized(int width, int height) final;
  void LostFocus() final;
  void GainedFocus(bool from_interstitial_ad) final;
  void AddInputEvent(std::unique_ptr<InputEvent> event) final;

  void CreateRendererInternal(RendererType type);

  void CreateTextureCompressors();

  void ContextLost();

  void ShowStats();

  Engine(const Engine&) = delete;
  Engine& operator=(const Engine&) = delete;
};

}  // namespace eng

#endif  // ENGINE_ENGINE_H
