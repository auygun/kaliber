#ifndef ENGINE_PLATFORM_PLATFORM_H
#define ENGINE_PLATFORM_PLATFORM_H

#include <array>
#include <cstdint>
#include <string>
#include <vector>

#include "engine/input_codes.h"

#if defined(OS_ANDROID)
#include "base/vecmath.h"
struct android_app;
struct AInputEvent;
struct ANativeWindow;
#else
struct GLFWcursor;
struct GLFWwindow;
#endif

namespace eng {

class PlatformObserver;

class Platform {
 public:
#if defined(OS_ANDROID)
  Platform(android_app* app);
#else
  Platform();
#endif
  ~Platform();

  void SetMainArgs(int argc, char** argv) {
    argc_ = argc;
    argv_ = argv;
  }

  int GetMainArgC() const { return argc_; }
  char** GetMainArgV() const { return argv_; }

  void CreateMainWindow(int width = -1, int height = -1);

  void Update(double wait_timeout = 0);

  void Exit();

  void SetObserver(PlatformObserver* observer) { observer_ = observer; }

  bool should_exit() const { return should_exit_; }

  // Paths the asset loader and save games are resolved against. On desktop
  // these all point at the directory containing the executable.
  const std::string& GetRootPath() const { return root_path_; }
  const std::string& GetDataPath() const { return data_path_; }
  const std::string& GetSharedDataPath() const { return shared_data_path_; }

  bool mobile_device() const { return mobile_device_; }

  int GetDeviceDpi() const { return device_dpi_; }

  // Mobile-only hooks. No-ops on desktop.
  void Vibrate(int duration);
  void ShowInterstitialAd();
  void ShareFile(const std::string& file_name);
  void SetKeepScreenOn(bool keep_screen_on);

  // True when the focus that is being regained follows an interstitial ad.
  // Always false on desktop.
  bool gained_focus_from_interstitial_ad() const {
    return gained_focus_from_interstitial_ad_;
  }

  // Input state ---------------------------------------------------------

  int GetMouseX() const { return mouse_x_; }
  int GetMouseY() const { return mouse_y_; }

  bool IsCursorInside() const { return cursor_inside_; }

  float GetMouseScrollYDelta() const { return mouse_scroll_y_delta_; }
  float GetMouseScrollXDelta() const { return mouse_scroll_x_delta_; }

  bool IsMouseButtonDown(MouseButton button) const {
    return mouse_buttons_down_[static_cast<int>(button)];
  }

  bool IsKeyDown(Key key) const { return keys_down_[static_cast<int>(key)]; }
  bool IsAnyKeyDown() const {
    for (auto k : keys_down_)
      if (k)
        return true;
    return false;
  }
  bool IsAnyMouseButtonDown() const {
    for (auto b : mouse_buttons_down_)
      if (b)
        return true;
    return false;
  }

  struct MouseButtonEvent {
    MouseButton button;
    bool pressed;
  };

  // Retrieve mouse button events queued this frame.
  const std::vector<MouseButtonEvent>& GetMouseButtonEvents() const {
    return mouse_button_events_;
  }

  // Retrieve the characters typed this frame.
  const std::vector<unsigned int>& GetInputCharacters() const {
    return input_characters_;
  }

  // Window --------------------------------------------------------------

  // Window size in window units. This is NOT the framebuffer size; the two
  // differ under display scaling. See Renderer::GetFramebufferWidth().
  int GetWindowWidth() const;
  int GetWindowHeight() const;

  void SetMouseCursor(int cursor);

  void SetClipboardText(const char* text);
  const char* GetClipboardText();

  // Primary selection (middle-click paste). An X11 concept; a no-op on
  // platforms that have no equivalent.
  void SetPrimarySelection(const char* text);
  const char* GetPrimarySelection();

  // The native window handle, for the renderer to create a surface from.
#if defined(OS_ANDROID)
  ANativeWindow* GetWindow();
#else
  GLFWwindow* GetWindow();
#endif

 private:
  bool mobile_device_ = false;
  int device_dpi_ = 100;
  std::string root_path_;
  std::string data_path_;
  std::string shared_data_path_;

  bool should_exit_ = false;
  bool cursor_inside_ = false;
  bool gained_focus_from_interstitial_ad_ = false;

  PlatformObserver* observer_ = nullptr;

  // Input state tracking
  int mouse_x_{0};
  int mouse_y_{0};
  float mouse_scroll_y_delta_{0.0f};
  float mouse_scroll_x_delta_{0.0f};
  std::array<bool, static_cast<int>(MouseButton::MaxButtons)>
      mouse_buttons_down_{};
  std::array<bool, static_cast<int>(Key::MaxKeys)> keys_down_{};
  // Buffer to store mouse button events this frame
  std::vector<MouseButtonEvent> mouse_button_events_;
  // Buffer to store UTF-32 characters typed this frame
  std::vector<unsigned int> input_characters_;

  int argc_ = 0;
  char** argv_ = nullptr;

#if defined(OS_ANDROID)

  android_app* app_ = nullptr;

  bool has_focus_ = false;

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
  static constexpr int kCursorCount = 11;
  GLFWcursor* cursors_[kCursorCount] = {};

  // Framebuffer size reported by GLFW's callback, applied in Update().
  int pending_width_ = 0;
  int pending_height_ = 0;
  bool has_pending_resize_ = false;

#endif  // defined(OS_ANDROID)

  Platform(const Platform&) = delete;
  Platform& operator=(const Platform&) = delete;
};

}  // namespace eng

#endif  // ENGINE_PLATFORM_PLATFORM_H
