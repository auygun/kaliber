#ifndef ENGINE_H
#define ENGINE_H

#include <deque>
#include <functional>
#include <list>
#include <memory>
#include <unordered_map>

#include "../base/random.h"
#include "../base/vecmath.h"
#include "audio/audio_forward.h"
#include "persistent_data.h"
#include "platform/platform_forward.h"
#include "renderer/render_resource.h"

class TextureCompressor;

namespace eng {

class Animator;
class AudioResource;
class Font;
class Game;
class Drawable;
class InputEvent;
class Image;
class ImageQuad;
class Renderer;
class Geometry;
class Shader;
class Texture;

class Engine {
 public:
  using CreateImageCB = std::function<std::unique_ptr<Image>()>;

  Engine(Platform* platform, Renderer* renderer, Audio* audio);
  ~Engine();

  static Engine& Get();

  bool Initialize();

  void Shutdown();

  void Update(float delta_time);
  void Draw(float frame_frac);

  void LostFocus();
  void GainedFocus(bool from_interstitial_ad);

  void AddDrawable(Drawable* drawable);
  void RemoveDrawable(Drawable* drawable);

  void AddAnimator(Animator* animator);
  void RemoveAnimator(Animator* animator);

  void Exit();

  // Convert size from pixels to viewport scale.
  base::Vector2f ToScale(const base::Vector2f& vec);

  // Convert position form pixels to viewport coordinates.
  base::Vector2f ToPosition(const base::Vector2f& vec);

  template <typename T>
  std::unique_ptr<T> CreateRenderResource() {
    RenderResourceFactory<T> factory;
    std::unique_ptr<RenderResource> resource =
        CreateRenderResourceInternal(factory);
    return std::unique_ptr<T>(static_cast<T*>(resource.release()));
  }

  void SetImageSource(const std::string& asset_name,
                      const std::string& file_name,
                      bool persistent = false);
  void SetImageSource(const std::string& asset_name,
                      CreateImageCB create_image,
                      bool persistent = false);

  void RefreshImage(const std::string& asset_name);

  std::shared_ptr<Texture> GetTexture(const std::string& asset_name);

  std::unique_ptr<AudioResource> CreateAudioResource();

  void AddInputEvent(std::unique_ptr<InputEvent> event);
  std::unique_ptr<InputEvent> GetNextInputEvent();

  void StartRecording(const Json::Value& payload);
  void EndRecording(const std::string file_name);

  bool Replay(const std::string file_name, Json::Value& payload);

  // Vibrate (if supported by the platform) for the specified duration.
  void Vibrate(int duration);

  void ShowInterstitialAd();

  void ShareFile(const std::string& file_name);

  void SetKeepScreenOn(bool keep_screen_on);

  void SetImageDpi(float dpi) { image_dpi_ = dpi; }

  void SetEnableAudio(bool enable);

  void SetEnableVibration(bool enable) { vibration_enabled_ = enable; }

  // Access to the render resources.
  Geometry* GetQuad() { return quad_.get(); }
  Shader* GetPassThroughShader() { return pass_through_shader_.get(); }
  Shader* GetSolidShader() { return solid_shader_.get(); }

  const Font* GetSystemFont() { return system_font_.get(); }

  base::Random& GetRandomGenerator() { return random_; }

  TextureCompressor* GetTextureCompressor(bool opacity);

  Game* GetGame() { return game_.get(); }

  // Return screen width/height in pixels.
  int GetScreenWidth() const;
  int GetScreenHeight() const;

  // Return screen size in viewport scale.
  base::Vector2f GetScreenSize() const { return screen_size_; }

  const base::Matrix4f& GetProjectionMatrix() const { return projection_; }

  int GetDeviceDpi() const;

  float GetImageScaleFactor() const;

  const std::string& GetRootPath() const;

  const std::string& GetDataPath() const;

  const std::string& GetSharedDataPath() const;

  int GetAudioHardwareSampleRate();

  bool IsMobile() const;

  float seconds_accumulated() const { return seconds_accumulated_; }

  float image_dpi() const { return image_dpi_; }

  float time_step() { return time_step_; }

  int fps() const { return fps_; }

 private:
  // Class holding information about texture resources managed by engine.
  // Texture is created from the image returned by create_image callback.
  struct TextureResource {
    std::shared_ptr<Texture> texture;
    CreateImageCB create_image;
    bool persistent = false;
  };

  static Engine* singleton;

  Platform* platform_ = nullptr;

  Renderer* renderer_ = nullptr;

  Audio* audio_ = nullptr;

  std::unique_ptr<Game> game_;

  std::unique_ptr<Geometry> quad_;
  std::unique_ptr<Shader> pass_through_shader_;
  std::unique_ptr<Shader> solid_shader_;

  base::Vector2f screen_size_ = {0, 0};
  base::Matrix4f projection_;

  std::unique_ptr<Font> system_font_;

  std::unique_ptr<TextureCompressor> tex_comp_opaque_;
  std::unique_ptr<TextureCompressor> tex_comp_alpha_;

  std::list<Drawable*> drawables_;

  std::list<Animator*> animators_;

  // Textures mapped by asset name.
  std::unordered_map<std::string, TextureResource> textures_;

  std::unique_ptr<ImageQuad> stats_;

  float fps_seconds_ = 0;
  int fps_ = 0;

  float seconds_accumulated_ = 0.0f;

  float time_step_ = 1.0f / 60.0f;
  size_t tick_ = 0;

  float image_dpi_ = 200;

  bool vibration_enabled_ = true;

  std::deque<std::unique_ptr<InputEvent>> input_queue_;

  PersistentData replay_data_;
  bool recording_ = false;
  bool replaying_ = false;
  int replay_index_ = 0;

  base::Random random_;

  std::unique_ptr<RenderResource> CreateRenderResourceInternal(
      RenderResourceFactoryBase& factory);

  void ContextLost();

  bool CreateRenderResources();

  void SetSatsVisible(bool visible);
  std::unique_ptr<Image> PrintStats();

  Engine(const Engine&) = delete;
  Engine& operator=(const Engine&) = delete;
};

}  // namespace eng

#endif  // ENGINE_H
