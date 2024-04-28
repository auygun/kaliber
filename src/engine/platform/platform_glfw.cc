#include "engine/platform/platform.h"

#include <limits.h>
#include <stdio.h>
#include <cstring>
#include <memory>

#include "base/log.h"
#include "base/vecmath.h"
#include "engine/input_event.h"
#include "engine/platform/platform_observer.h"

using namespace base;

namespace eng {

void KaliberMain(Platform* platform);

Platform::Platform() {
  LOG(0) << "Initializing platform.";

  CHECK(glfwInit()) << "GLFW failed to init!";

  root_path_ = "./";
  data_path_ = "./";
  shared_data_path_ = "./";

  char dest[PATH_MAX];
  memset(dest, 0, sizeof(dest));
  if (readlink("/proc/self/exe", dest, PATH_MAX) > 0) {
    std::string path = dest;
    std::size_t last_slash_pos = path.find_last_of('/');
    if (last_slash_pos != std::string::npos)
      path = path.substr(0, last_slash_pos + 1);

    root_path_ = path;
    data_path_ = path;
    shared_data_path_ = path;
  }

  LOG(0) << "Root path: " << root_path_.c_str();
  LOG(0) << "Data path: " << data_path_.c_str();
  LOG(0) << "Shared data path: " << shared_data_path_.c_str();
}

void Platform::CreateMainWindow() {
  glfwDefaultWindowHints();
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  window_ = glfwCreateWindow(800, 1205, "kaliber", nullptr, nullptr);
}

Platform::~Platform() {
  LOG(0) << "Shutting down platform.";
  DestroyWindow();
  glfwTerminate();
}

void Platform::Update() {}

void Platform::Exit() {
  should_exit_ = true;
}

void Platform::Vibrate(int duration) {}

void Platform::ShowInterstitialAd() {}

void Platform::ShareFile(const std::string& file_name) {}

void Platform::SetKeepScreenOn(bool keep_screen_on) {}

void Platform::DestroyWindow() {
  if (window_) {
    glfwDestroyWindow(window_);
    window_ = nullptr;
  }
}

GLFWwindow* Platform::GetWindow() {
  return window_;
}

}  // namespace eng

int main(int argc, char** argv) {
  eng::Platform platform;
  eng::KaliberMain(&platform);
  return 0;
}
