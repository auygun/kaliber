#ifndef ENGINE_PLATFORM_PLATFORM_H
#define ENGINE_PLATFORM_PLATFORM_H

#include <array>
#include <cstdint>
#include <string>
#include <vector>

#if defined(__ANDROID__)
#include "../../base/vecmath.h"
struct android_app;
struct AInputEvent;
struct ANativeWindow;
#else
struct GLFWwindow;
#endif

#include "engine/input_codes.h"

namespace eng {

class PlatformObserver;

class Platform {
 public:
#if defined(__ANDROID__)
  Platform(android_app* app);
#else
  Platform();
#endif
  ~Platform();

  void CreateMainWindow(bool use_opengl = false,
                        int width = 0,
                        int height = 0);

  void Update();

  void Exit();

  void SetObserver(PlatformObserver* observer) { observer_ = observer; }

  void Vibrate(int duration);
  void ShowInterstitialAd();
  void ShareFile(const std::string& file_name);
  void SetKeepScreenOn(bool keep_screen_on);

#if defined(__ANDROID__)
  ANativeWindow* GetWindow();
#else
  void SetMainArgs(int argc, char** argv);

  GLFWwindow* GetWindow();
#endif

  const std::string& GetRootPath() const { return root_path_; }

  const std::string& GetDataPath() const { return data_path_; }

  const std::string& GetSharedDataPath() const { return shared_data_path_; }

  bool mobile_device() const { return mobile_device_; }

  bool should_exit() const { return should_exit_; }

  int GetMouseX() const { return mouse_x_; }
  int GetMouseY() const { return mouse_y_; }

  float GetMouseScrollDelta() const { return mouse_scroll_y_delta_; }

  bool IsMouseButtonDown(MouseButton button) const {
    return mouse_buttons_down_[static_cast<int>(button)];
  }

  bool IsKeyDown(Key key) const { return keys_down_[static_cast<int>(key)]; }

  const std::vector<unsigned int>& GetInputCharacters() const {
    return input_characters_;
  }

 private:
  bool mobile_device_ = false;
  std::string root_path_;
  std::string data_path_;
  std::string shared_data_path_;

  bool should_exit_ = false;

  PlatformObserver* observer_ = nullptr;

  int mouse_x_{0};
  int mouse_y_{0};
  float mouse_scroll_x_delta_{0.0f};
  float mouse_scroll_y_delta_{0.0f};
  std::array<bool, static_cast<int>(MouseButton::MaxButtons)>
      mouse_buttons_down_{};
  std::array<bool, static_cast<int>(Key::MaxKeys)> keys_down_{};
  std::vector<unsigned int> input_characters_;

#if defined(__ANDROID__)

  bool has_focus_ = false;
  int device_dpi_ = 100;

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

#else

  GLFWwindow* window_ = nullptr;
  int pending_width_ = 0;
  int pending_height_ = 0;
  bool has_pending_resize_ = false;

  void PlatformInit();
  void PlatformShutdown();

#endif

  Platform(const Platform&) = delete;
  Platform& operator=(const Platform&) = delete;
};

}  // namespace eng

#endif  // ENGINE_PLATFORM_PLATFORM_H
