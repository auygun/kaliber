#ifndef ENGINE_PLATFORM_PLATFORM_H
#define ENGINE_PLATFORM_PLATFORM_H

#include <memory>
#include <string>

#include "base/thread_pool.h"
#include "base/timer.h"

#if defined(__ANDROID__)

#include "../../base/vecmath.h"

struct android_app;
struct AInputEvent;
struct ANativeWindow;

#elif defined(__linux__)

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#endif

namespace eng {

class AudioDriver;
class Renderer;
class Engine;

class Platform {
 public:
  Platform();
  ~Platform();

#if defined(__ANDROID__)
  void Initialize(android_app* app);
#elif defined(__linux__)
  void Initialize();
#endif

  void Shutdown();

  void Update();

  void Exit();

  Renderer* SwitchRenderer(bool vulkan);

  void Vibrate(int duration);

  void ShowInterstitialAd();

  void ShareFile(const std::string& file_name);

  void SetKeepScreenOn(bool keep_screen_on);

  void RunMainLoop();

  int GetDeviceDpi() const { return device_dpi_; }

  const std::string& GetRootPath() const { return root_path_; }

  const std::string& GetDataPath() const { return data_path_; }

  const std::string& GetSharedDataPath() const { return shared_data_path_; }

  bool mobile_device() const { return mobile_device_; }

 protected:
  base::Timer timer_;

  bool mobile_device_ = false;
  int device_dpi_ = 100;
  std::string root_path_;
  std::string data_path_;
  std::string shared_data_path_;

  bool has_focus_ = false;
  bool should_exit_ = false;

  std::unique_ptr<AudioDriver> audio_driver_;
  std::unique_ptr<Renderer> renderer_;
  std::unique_ptr<Engine> engine_;

  base::ThreadPool thread_pool_;

#if defined(__ANDROID__)

  android_app* app_ = nullptr;

  base::Vector2f pointer_pos_[2] = {{0, 0}, {0, 0}};
  bool pointer_down_[2] = {false, false};

  static int32_t HandleInput(android_app* app, AInputEvent* event);
  static void HandleCmd(android_app* app, int32_t cmd);

  using PFN_ANativeWindow_setFrameRate = int32_t (*)(ANativeWindow* window,
                                                     float frameRate,
                                                     int8_t compatibility);
  using PFN_ANativeWindow_setFrameRateWithChangeStrategy =
      int32_t (*)(ANativeWindow* window,
                  float frameRate,
                  int8_t compatibility,
                  int8_t changeFrameRateStrategy);

  PFN_ANativeWindow_setFrameRate ANativeWindow_setFrameRate = nullptr;
  PFN_ANativeWindow_setFrameRateWithChangeStrategy
      ANativeWindow_setFrameRateWithChangeStrategy = nullptr;

  void SetFrameRate(float frame_rate);

#elif defined(__linux__)

  Display* display_ = nullptr;
  Window window_ = 0;

  bool CreateWindow(int width, int height);
  void DestroyWindow();

#endif

  void InitializeCommon();
  void ShutdownCommon();

  bool InitializeRenderer();

  Platform(const Platform&) = delete;
  Platform& operator=(const Platform&) = delete;
};

}  // namespace eng

#endif  // ENGINE_PLATFORM_PLATFORM_H
