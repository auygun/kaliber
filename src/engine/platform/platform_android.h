#ifndef PLATFORM_ANDROID_H
#define PLATFORM_ANDROID_H

#include <memory>

#include "platform_base.h"

struct android_app;
struct AInputEvent;

namespace ndk_helper {
class TapDetector;
class PinchDetector;
class DragDetector;
}  // namespace ndk_helper

namespace eng {

class PlatformAndroid : public PlatformBase {
 public:
  PlatformAndroid();
  ~PlatformAndroid();

  void Initialize(android_app* app);

  void Update();

  void Exit();

 private:
  android_app* app_ = nullptr;

  std::unique_ptr<ndk_helper::TapDetector> tap_detector_;
  std::unique_ptr<ndk_helper::PinchDetector> pinch_detector_;
  std::unique_ptr<ndk_helper::DragDetector> drag_detector_;

  static int32_t HandleInput(android_app* app, AInputEvent* event);
  static void HandleCmd(android_app* app, int32_t cmd);
};

}  // namespace eng

#endif  // PLATFORM_ANDROID_H
