#ifndef PLATFORM_H
#define PLATFORM_H

#include <exception>
#include <memory>
#include <string>

#include "../../base/timer.h"

#if defined(__ANDROID__)
struct android_app;
struct AInputEvent;

namespace ndk_helper {
class TapDetector;
class PinchDetector;
class DragDetector;
}  // namespace ndk_helper
#endif

namespace eng {

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

  void RunMainLoop();

  void Exit();

  Renderer* GetRenderer() { return renderer_.get(); }

  int GetDeviceDpi() const { return device_dpi_; }

  const std::string& GetRootPath() const { return root_path_; }

  bool mobile_device() const { return mobile_device_; }

  static class InternalError : public std::exception {
  } internal_error;

 private:
  base::Timer timer_;

  bool mobile_device_ = false;
  int device_dpi_ = 200;
  std::string root_path_;

  bool has_focus_ = false;
  bool should_exit_ = false;

  std::unique_ptr<Renderer> renderer_;
  std::unique_ptr<Engine> engine_;

#if defined(__ANDROID__)
  android_app* app_ = nullptr;

  std::unique_ptr<ndk_helper::TapDetector> tap_detector_;
  std::unique_ptr<ndk_helper::PinchDetector> pinch_detector_;
  std::unique_ptr<ndk_helper::DragDetector> drag_detector_;

  static int32_t HandleInput(android_app* app, AInputEvent* event);
  static void HandleCmd(android_app* app, int32_t cmd);
#endif

  Platform(const Platform&) = delete;
  Platform& operator=(const Platform&) = delete;
};

}  // namespace eng

#endif  // PLATFORM_H
