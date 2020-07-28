#ifndef PLATFORM_ANDROID_H
#define PLATFORM_ANDROID_H

#include <memory>

#include "platform_base.h"

struct android_app;
struct AInputEvent;

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

  static int32_t HandleInput(android_app* app, AInputEvent* event);
  static void HandleCmd(android_app* app, int32_t cmd);
};

}  // namespace eng

#endif  // PLATFORM_ANDROID_H
