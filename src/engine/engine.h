#ifndef ENGINE_ENGINE_H
#define ENGINE_ENGINE_H

#include <deque>
#include <functional>
#include <list>
#include <memory>
#include <unordered_map>

#include "base/random.h"
#include "base/thread_pool.h"
#include "base/vecmath.h"
#include "engine/imgui_backend.h"
#include "engine/persistent_data.h"
#include "engine/platform/platform_observer.h"
#include "engine/renderer/geometry.h"
#include "engine/renderer/shader.h"
#include "engine/renderer/texture.h"

class TextureCompressor;

namespace eng {

class Animator;
class AudioBus;
class AudioMixer;
class Drawable;
class Font;
class Game;
class Image;
class InputEvent;
class Platform;
class Renderer;
enum class RendererType;

class Engine : public PlatformObserver {
 public:
  using CreateImageCB = std::function<std::unique_ptr<Image>()>;

  Engine(Platform* platform);
  ~Engine() noexcept override;

  static Engine& Get();

  void Run();

  void AddDrawable(Drawable* drawable);
  void RemoveDrawable(Drawable* drawable);

  void AddAnimator(Animator* animator);
  void RemoveAnimator(Animator* animator);

  void CreateRenderer(RendererType type);
  RendererType GetRendererType();

  void Exit();

  // Convert size from pixels to viewport scale.
  base::Vector2f ToScale(const base::Vector2f& vec);

  // Convert position form pixels to viewport coordinates.
  base::Vector2f ToPosition(const base::Vector2f& vec);

  void SetImageSource(const std::string& asset_name,
                      const std::string& file_name,
                      bool persistent = false);
  void SetImageSource(const std::string& asset_name,
                      CreateImageCB create_image,
                      bool persistent = false);

  void RefreshImage(const std::string& asset_name);
  std::shared_ptr<Texture> AcquireTexture(const std::string& asset_name);

  void SetShaderSource(const std::string& asset_name,
                       const std::string& file_name);
  std::shared_ptr<Shader> GetShader(const std::string& asset_name);

  void AsyncLoadSound(const std::string& asset_name,
                      const std::string& file_name,
                      bool stream = false);
  std::shared_ptr<AudioBus> GetAudioBus(const std::string& asset_name);

  std::unique_ptr<InputEvent> GetNextInputEvent();

  void StartRecording(const Json::Value& payload);
  void EndRecording(const std::string file_name);

  bool Replay(const std::string file_name, Json::Value& payload);

  // Vibrate (if supported by the platform) for the specified duration.
  void Vibrate(int duration);

  void ShowInterstitialAd();

  void ShareFile(const std::string& file_name);

  void SetKeepScreenOn(bool keep_screen_on);

  void SetEnableAudio(bool enable);

  void SetEnableVibration(bool enable) { vibration_enabled_ = enable; }

  AudioMixer* GetAudioMixer() { return audio_mixer_.get(); }

  // Access to the render resources.
  Geometry& GetQuad() { return quad_; }
  Shader& GetPassThroughShader() { return pass_through_shader_; }
  Shader& GetSolidShader() { return solid_shader_; }

  const Font* GetSystemFont() { return system_font_.get(); }

  std::unique_ptr<Image> Print(const std::string& text,
                               base::Vector4f bg_color);

  base::Randomf& GetRandomGenerator() { return random_; }

  TextureCompressor* GetTextureCompressor(bool opacity);

  Game* GetGame() { return game_.get(); }

  // Return screen width/height in pixels.
  int GetScreenWidth() const;
  int GetScreenHeight() const;

  // Return screen size in viewport scale.
  base::Vector2f GetScreenSize() const { return screen_size_; }

  const base::Matrix4f& GetProjectionMatrix() const { return projection_; }

  float GetImageScaleFactor() const;

  const std::string& GetRootPath() const;

  const std::string& GetDataPath() const;

  const std::string& GetSharedDataPath() const;

  size_t GetAudioHardwareSampleRate();

  bool IsMobile() const;

  float seconds_accumulated() const { return seconds_accumulated_; }

  float time_step() { return time_step_; }

  int fps() const { return fps_; }

 private:
  // Class holding information about texture resources managed by engine.
  // Texture is created from the image returned by create_image callback.
  struct TextureResource {
    std::shared_ptr<Texture> persistent_ptr;
    std::weak_ptr<Texture> texture;
    CreateImageCB create_image;
  };

  // Class holding information about shader resources managed by engine.
  struct ShaderResource {
    std::weak_ptr<Shader> shader;
    std::string file_name;
  };

  static Engine* singleton;

  Platform* platform_ = nullptr;

  std::unique_ptr<Renderer> renderer_;
  std::unique_ptr<AudioMixer> audio_mixer_;
  std::unique_ptr<Game> game_;

  Geometry quad_;
  Shader pass_through_shader_;
  Shader solid_shader_;

  base::Vector2f screen_size_ = {0, 0};
  base::Matrix4f projection_;

  std::unique_ptr<Font> system_font_;

  std::unique_ptr<TextureCompressor> tex_comp_opaque_;
  std::unique_ptr<TextureCompressor> tex_comp_alpha_;

  std::list<Drawable*> drawables_;

  std::list<Animator*> animators_;

  // Resources mapped by asset name.
  std::unordered_map<std::string, TextureResource> textures_;
  std::unordered_map<std::string, ShaderResource> shaders_;
  std::unordered_map<std::string, std::shared_ptr<AudioBus>> audio_buses_;

  size_t async_work_count_ = 0;

  bool stats_visible_ = false;

  ImguiBackend imgui_backend_;

  float fps_seconds_ = 0;
  int fps_ = 0;

  float seconds_accumulated_ = 0.0f;
  float time_step_ = 1.0f / 60.0f;
  size_t tick_ = 0;

  bool vibration_enabled_ = true;

  std::deque<std::unique_ptr<InputEvent>> input_queue_;

  PersistentData replay_data_;
  bool recording_ = false;
  bool replaying_ = false;
  unsigned int replay_index_ = 0;

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

  void CreateProjectionMatrix();

  void ContextLost();

  void CreateRenderResources();

  void WaitForAsyncWork();

  void ShowStats();

  Engine(const Engine&) = delete;
  Engine& operator=(const Engine&) = delete;
};

}  // namespace eng

#endif  // ENGINE_ENGINE_H
