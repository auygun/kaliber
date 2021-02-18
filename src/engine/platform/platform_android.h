#ifndef PLATFORM_ANDROID_H
#define PLATFORM_ANDROID_H

#include "../../base/vecmath.h"
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

  void Vibrate(int duration);

  void ShowInterstitialAd();

  void ShareFile(const std::string& file_name);

  void SetKeepScreenOn(bool keep_screen_on);

 private:
  android_app* app_ = nullptr;

  base::Vector2f pointer_pos_[2] = {{0, 0}, {0, 0}};
  bool pointer_down_[2] = {false, false};

  static int32_t HandleInput(android_app* app, AInputEvent* event);
  static void HandleCmd(android_app* app, int32_t cmd);
};

}  // namespace eng

#endif  // PLATFORM_ANDROID_H
