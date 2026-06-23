#include "engine/platform/platform.h"

#include "third_party/glfw/glfw/include/GLFW/glfw3.h"

#include "base/log.h"
#include "engine/platform/platform_observer.h"

#if defined(OS_LINUX)
#include <limits.h>
#include <unistd.h>
#include <cstring>
#elif defined(OS_WIN)
#include <windows.h>
#endif

namespace eng {

namespace {

Key TranslateGLFWKey(int key) {
  switch (key) {
    case GLFW_KEY_A:
      return Key::A;
    case GLFW_KEY_B:
      return Key::B;
    case GLFW_KEY_C:
      return Key::C;
    case GLFW_KEY_D:
      return Key::D;
    case GLFW_KEY_E:
      return Key::E;
    case GLFW_KEY_F:
      return Key::F;
    case GLFW_KEY_G:
      return Key::G;
    case GLFW_KEY_H:
      return Key::H;
    case GLFW_KEY_I:
      return Key::I;
    case GLFW_KEY_J:
      return Key::J;
    case GLFW_KEY_K:
      return Key::K;
    case GLFW_KEY_L:
      return Key::L;
    case GLFW_KEY_M:
      return Key::M;
    case GLFW_KEY_N:
      return Key::N;
    case GLFW_KEY_O:
      return Key::O;
    case GLFW_KEY_P:
      return Key::P;
    case GLFW_KEY_Q:
      return Key::Q;
    case GLFW_KEY_R:
      return Key::R;
    case GLFW_KEY_S:
      return Key::S;
    case GLFW_KEY_T:
      return Key::T;
    case GLFW_KEY_U:
      return Key::U;
    case GLFW_KEY_V:
      return Key::V;
    case GLFW_KEY_W:
      return Key::W;
    case GLFW_KEY_X:
      return Key::X;
    case GLFW_KEY_Y:
      return Key::Y;
    case GLFW_KEY_Z:
      return Key::Z;

    case GLFW_KEY_0:
      return Key::Num0;
    case GLFW_KEY_1:
      return Key::Num1;
    case GLFW_KEY_2:
      return Key::Num2;
    case GLFW_KEY_3:
      return Key::Num3;
    case GLFW_KEY_4:
      return Key::Num4;
    case GLFW_KEY_5:
      return Key::Num5;
    case GLFW_KEY_6:
      return Key::Num6;
    case GLFW_KEY_7:
      return Key::Num7;
    case GLFW_KEY_8:
      return Key::Num8;
    case GLFW_KEY_9:
      return Key::Num9;

    case GLFW_KEY_F1:
      return Key::F1;
    case GLFW_KEY_F2:
      return Key::F2;
    case GLFW_KEY_F3:
      return Key::F3;
    case GLFW_KEY_F4:
      return Key::F4;
    case GLFW_KEY_F5:
      return Key::F5;
    case GLFW_KEY_F6:
      return Key::F6;
    case GLFW_KEY_F7:
      return Key::F7;
    case GLFW_KEY_F8:
      return Key::F8;
    case GLFW_KEY_F9:
      return Key::F9;
    case GLFW_KEY_F10:
      return Key::F10;
    case GLFW_KEY_F11:
      return Key::F11;
    case GLFW_KEY_F12:
      return Key::F12;
    case GLFW_KEY_F13:
      return Key::F13;
    case GLFW_KEY_F14:
      return Key::F14;
    case GLFW_KEY_F15:
      return Key::F15;
    case GLFW_KEY_F16:
      return Key::F16;
    case GLFW_KEY_F17:
      return Key::F17;
    case GLFW_KEY_F18:
      return Key::F18;
    case GLFW_KEY_F19:
      return Key::F19;
    case GLFW_KEY_F20:
      return Key::F20;
    case GLFW_KEY_F21:
      return Key::F21;
    case GLFW_KEY_F22:
      return Key::F22;
    case GLFW_KEY_F23:
      return Key::F23;
    case GLFW_KEY_F24:
      return Key::F24;

    case GLFW_KEY_ESCAPE:
      return Key::Escape;
    case GLFW_KEY_SPACE:
      return Key::Space;
    case GLFW_KEY_ENTER:
      return Key::Enter;
    case GLFW_KEY_TAB:
      return Key::Tab;
    case GLFW_KEY_BACKSPACE:
      return Key::Backspace;

    case GLFW_KEY_UP:
      return Key::Up;
    case GLFW_KEY_DOWN:
      return Key::Down;
    case GLFW_KEY_LEFT:
      return Key::Left;
    case GLFW_KEY_RIGHT:
      return Key::Right;
    case GLFW_KEY_PAGE_UP:
      return Key::PageUp;
    case GLFW_KEY_PAGE_DOWN:
      return Key::PageDown;
    case GLFW_KEY_HOME:
      return Key::Home;
    case GLFW_KEY_END:
      return Key::End;
    case GLFW_KEY_INSERT:
      return Key::Insert;
    case GLFW_KEY_DELETE:
      return Key::Delete;

    case GLFW_KEY_LEFT_SHIFT:
      return Key::ShiftLeft;
    case GLFW_KEY_RIGHT_SHIFT:
      return Key::ShiftRight;
    case GLFW_KEY_LEFT_CONTROL:
      return Key::ControlLeft;
    case GLFW_KEY_RIGHT_CONTROL:
      return Key::ControlRight;
    case GLFW_KEY_LEFT_ALT:
      return Key::AltLeft;
    case GLFW_KEY_RIGHT_ALT:
      return Key::AltRight;
    case GLFW_KEY_LEFT_SUPER:
      return Key::SuperLeft;
    case GLFW_KEY_RIGHT_SUPER:
      return Key::SuperRight;
    case GLFW_KEY_MENU:
      return Key::Menu;

    case GLFW_KEY_APOSTROPHE:
      return Key::Apostrophe;
    case GLFW_KEY_COMMA:
      return Key::Comma;
    case GLFW_KEY_MINUS:
      return Key::Minus;
    case GLFW_KEY_PERIOD:
      return Key::Period;
    case GLFW_KEY_SLASH:
      return Key::Slash;
    case GLFW_KEY_SEMICOLON:
      return Key::Semicolon;
    case GLFW_KEY_EQUAL:
      return Key::Equal;
    case GLFW_KEY_LEFT_BRACKET:
      return Key::LeftBracket;
    case GLFW_KEY_BACKSLASH:
      return Key::Backslash;
    case GLFW_KEY_RIGHT_BRACKET:
      return Key::RightBracket;
    case GLFW_KEY_GRAVE_ACCENT:
      return Key::GraveAccent;

    case GLFW_KEY_CAPS_LOCK:
      return Key::CapsLock;
    case GLFW_KEY_SCROLL_LOCK:
      return Key::ScrollLock;
    case GLFW_KEY_NUM_LOCK:
      return Key::NumLock;
    case GLFW_KEY_PRINT_SCREEN:
      return Key::PrintScreen;
    case GLFW_KEY_PAUSE:
      return Key::Pause;

    case GLFW_KEY_KP_0:
      return Key::Keypad0;
    case GLFW_KEY_KP_1:
      return Key::Keypad1;
    case GLFW_KEY_KP_2:
      return Key::Keypad2;
    case GLFW_KEY_KP_3:
      return Key::Keypad3;
    case GLFW_KEY_KP_4:
      return Key::Keypad4;
    case GLFW_KEY_KP_5:
      return Key::Keypad5;
    case GLFW_KEY_KP_6:
      return Key::Keypad6;
    case GLFW_KEY_KP_7:
      return Key::Keypad7;
    case GLFW_KEY_KP_8:
      return Key::Keypad8;
    case GLFW_KEY_KP_9:
      return Key::Keypad9;
    case GLFW_KEY_KP_DECIMAL:
      return Key::KeypadDecimal;
    case GLFW_KEY_KP_DIVIDE:
      return Key::KeypadDivide;
    case GLFW_KEY_KP_MULTIPLY:
      return Key::KeypadMultiply;
    case GLFW_KEY_KP_SUBTRACT:
      return Key::KeypadSubtract;
    case GLFW_KEY_KP_ADD:
      return Key::KeypadAdd;
    case GLFW_KEY_KP_ENTER:
      return Key::KeypadEnter;
    case GLFW_KEY_KP_EQUAL:
      return Key::KeypadEqual;

    default:
      return Key::Unknown;
  }
}

}  // namespace

void KaliberMain(Platform* platform);

Platform::Platform() = default;

Platform::~Platform() {
  DLOG(0) << "Shutting down platform.";
  PlatformShutdown();
  if (window_) {
    glfwDestroyWindow(window_);
    window_ = nullptr;
  }
  glfwTerminate();
}

void Platform::CreateMainWindow(bool use_opengl, int width, int height) {
  if (window_) {
    glfwDestroyWindow(window_);
    window_ = nullptr;
    keys_down_.fill(false);
    mouse_buttons_down_.fill(false);
  }

  if (!glfwInit()) {
    DLOG(0) << "Failed to initialize GLFW.";
    return;
  }

  glfwSetErrorCallback([](int error, const char* description) {
    DLOG(0) << "GLFW error " << error << ": " << description;
  });

  glfwDefaultWindowHints();
  if (!use_opengl) {
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  } else {
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  }
#if defined(OS_LINUX)
  glfwWindowHintString(GLFW_X11_CLASS_NAME, "kaliber");
  glfwWindowHintString(GLFW_X11_INSTANCE_NAME, "kaliber");
  glfwWindowHintString(GLFW_WAYLAND_APP_ID, "kaliber");
#endif

  if (width <= 0 || height <= 0) {
    int work_x, work_y, work_w, work_h;
    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    glfwGetMonitorWorkarea(monitor, &work_x, &work_y, &work_w, &work_h);
    float scale_x = 1, scale_y = 1;
    glfwGetMonitorContentScale(monitor, &scale_x, &scale_y);
    if (width <= 0)
      width = static_cast<int>(work_w * 3.0f / 4 / (scale_x > 0 ? scale_x : 1));
    if (height <= 0)
      height =
          static_cast<int>(work_h * 3.0f / 4 / (scale_y > 0 ? scale_y : 1));
  }

  window_ = glfwCreateWindow(width, height, "kaliber", nullptr, nullptr);
  CHECK(window_) << "Failed to create GLFW window.";
  glfwSetWindowSizeLimits(window_, 200, 200, GLFW_DONT_CARE, GLFW_DONT_CARE);

  glfwSetWindowUserPointer(window_, this);

  glfwSetKeyCallback(window_, [](GLFWwindow* w, int key, int /*scancode*/,
                                 int action, int /*mods*/) {
    auto* p = static_cast<Platform*>(glfwGetWindowUserPointer(w));
    Key translated = TranslateGLFWKey(key);
    if (translated != Key::Unknown) {
      p->keys_down_[static_cast<int>(translated)] =
          (action == GLFW_PRESS || action == GLFW_REPEAT);
    }
  });

  glfwSetCharCallback(window_, [](GLFWwindow* w, unsigned int codepoint) {
    auto* p = static_cast<Platform*>(glfwGetWindowUserPointer(w));
    p->input_characters_.push_back(codepoint);
  });

  glfwSetCursorPosCallback(
      window_, [](GLFWwindow* w, double xpos, double ypos) {
        auto* p = static_cast<Platform*>(glfwGetWindowUserPointer(w));
        p->mouse_x_ = static_cast<int>(xpos);
        p->mouse_y_ = static_cast<int>(ypos);
      });

  glfwSetMouseButtonCallback(
      window_, [](GLFWwindow* w, int button, int action, int /*mods*/) {
        auto* p = static_cast<Platform*>(glfwGetWindowUserPointer(w));
        MouseButton mb = MouseButton::Unknown;
        if (button == GLFW_MOUSE_BUTTON_LEFT)
          mb = MouseButton::Left;
        else if (button == GLFW_MOUSE_BUTTON_RIGHT)
          mb = MouseButton::Right;
        else if (button == GLFW_MOUSE_BUTTON_MIDDLE)
          mb = MouseButton::Middle;
        if (mb != MouseButton::Unknown)
          p->mouse_buttons_down_[static_cast<int>(mb)] = (action == GLFW_PRESS);
      });

  glfwSetScrollCallback(
      window_, [](GLFWwindow* w, double xoffset, double yoffset) {
        auto* p = static_cast<Platform*>(glfwGetWindowUserPointer(w));
        p->mouse_scroll_x_delta_ += static_cast<float>(xoffset);
        p->mouse_scroll_y_delta_ += static_cast<float>(yoffset);
      });

  glfwSetFramebufferSizeCallback(
      window_, [](GLFWwindow* w, int width, int height) {
        auto* p = static_cast<Platform*>(glfwGetWindowUserPointer(w));
        p->pending_width_ = width;
        p->pending_height_ = height;
        p->has_pending_resize_ = true;
      });

  glfwSetWindowFocusCallback(window_, [](GLFWwindow* w, int focused) {
    auto* p = static_cast<Platform*>(glfwGetWindowUserPointer(w));
    if (!p->observer_)
      return;
    if (focused)
      p->observer_->GainedFocus(false);
    else
      p->observer_->LostFocus();
  });

  glfwSetWindowCloseCallback(window_, [](GLFWwindow* w) {
    auto* p = static_cast<Platform*>(glfwGetWindowUserPointer(w));
    for (bool down : p->mouse_buttons_down_) {
      if (down) {
        glfwSetWindowShouldClose(w, GLFW_FALSE);
        return;
      }
    }
    p->should_exit_ = true;
  });

  observer_->OnWindowCreated();

  PlatformInit();
}

void Platform::Update() {
  mouse_scroll_y_delta_ = 0.0f;
  mouse_scroll_x_delta_ = 0.0f;
  input_characters_.clear();

  glfwPollEvents();

  if (has_pending_resize_) {
    has_pending_resize_ = false;
    if (observer_)
      observer_->OnWindowResized(pending_width_, pending_height_);
  }
  if (glfwWindowShouldClose(window_))
    should_exit_ = true;
}

void Platform::Exit() {
  glfwSetWindowShouldClose(window_, GLFW_TRUE);
  should_exit_ = true;
}

GLFWwindow* Platform::GetWindow() {
  return window_;
}

void Platform::Vibrate(int duration) {}

void Platform::ShowInterstitialAd() {}

void Platform::ShareFile(const std::string& file_name) {}

void Platform::SetKeepScreenOn(bool keep_screen_on) {}

// --- Platform-specific implementations ---

#if defined(OS_LINUX)

void Platform::SetMainArgs(int argc, char** argv) {
  root_path_ = "./";
  data_path_ = "./";
  shared_data_path_ = "./";

  char dest[PATH_MAX];
  memset(dest, 0, sizeof(dest));
  if (readlink("/proc/self/exe", dest, PATH_MAX) > 0) {
    std::string path = dest;
    auto last_slash = path.find_last_of('/');
    if (last_slash != std::string::npos)
      path = path.substr(0, last_slash + 1);
    root_path_ = path;
    data_path_ = path;
    shared_data_path_ = path;
  }

  DLOG(0) << "Root path: " << root_path_.c_str();
  DLOG(0) << "Data path: " << data_path_.c_str();
  DLOG(0) << "Shared data path: " << shared_data_path_.c_str();
}

void Platform::PlatformInit() {}

void Platform::PlatformShutdown() {}

#elif defined(OS_WIN)

void Platform::SetMainArgs(int argc, char** argv) {
  root_path_ = ".\\";
  data_path_ = ".\\";
  shared_data_path_ = ".\\";

  char dest[MAX_PATH];
  memset(dest, 0, sizeof(dest));
  if (GetModuleFileNameA(NULL, dest, MAX_PATH) > 0) {
    std::string path = dest;
    auto last_slash = path.find_last_of('\\');
    if (last_slash != std::string::npos)
      path = path.substr(0, last_slash + 1);
    root_path_ = path;
    data_path_ = path;
    shared_data_path_ = path;
  }

  DLOG(0) << "Root path: " << root_path_.c_str();
  DLOG(0) << "Data path: " << data_path_.c_str();
  DLOG(0) << "Shared data path: " << shared_data_path_.c_str();
}

void Platform::PlatformInit() {}

void Platform::PlatformShutdown() {}

#endif

}  // namespace eng

int main(int argc, char** argv) {
  eng::Platform platform;
  platform.SetMainArgs(argc, argv);
  eng::KaliberMain(&platform);
  return 0;
}
