#ifndef PLATFORM_LINUX_H
#define PLATFORM_LINUX_H

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include "platform_base.h"

namespace eng {

class PlatformLinux : public PlatformBase {
 public:
  PlatformLinux();
  ~PlatformLinux();

  void Initialize();

  void Shutdown();

  void Update();

  void Exit();

  void Vibrate(int duration) {}

  void ShowInterstitialAd() {}

  void ShareFile(const std::string& file_name) {}

  void SetKeepScreenOn(bool keep_screen_on) {}

 private:
  Display* display_ = nullptr;
  Window window_ = 0;

  bool CreateWindow(int width, int height);
  void DestroyWindow();
};

}  // namespace eng

#endif  // PLATFORM_LINUX_H
