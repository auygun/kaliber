#include "engine/platform/platform.h"

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

  root_path_ = "../../";
  LOG(0) << "Root path: " << root_path_.c_str();

  data_path_ = "./";
  LOG(0) << "Data path: " << data_path_.c_str();

  shared_data_path_ = "./";
  LOG(0) << "Shared data path: " << shared_data_path_.c_str();

  bool res = CreateWindow(800, 1205);
  CHECK(res) << "Failed to create window.";

  XSelectInput(display_, window_,
               KeyPressMask | Button1MotionMask | ButtonPressMask |
                   ButtonReleaseMask | FocusChangeMask | StructureNotifyMask);
  Atom WM_DELETE_WINDOW = XInternAtom(display_, "WM_DELETE_WINDOW", false);
  XSetWMProtocols(display_, window_, &WM_DELETE_WINDOW, 1);
}

Platform::~Platform() {
  LOG(0) << "Shutting down platform.";
  DestroyWindow();
}

void Platform::Update() {
  while (XPending(display_)) {
    XEvent e;
    XNextEvent(display_, &e);
    switch (e.type) {
      case KeyPress: {
        KeySym key = XLookupKeysym(&e.xkey, 0);
        auto input_event =
            std::make_unique<InputEvent>(InputEvent::kKeyPress, (char)key);
        observer_->AddInputEvent(std::move(input_event));
        // TODO: e.xkey.state & (ShiftMask | ControlMask | Mod1Mask | Mod4Mask))
        break;
      }
      case MotionNotify: {
        Vector2f v(e.xmotion.x, e.xmotion.y);
        auto input_event =
            std::make_unique<InputEvent>(InputEvent::kDrag, 0, v);
        observer_->AddInputEvent(std::move(input_event));
        break;
      }
      case ButtonPress: {
        if (e.xbutton.button == 1) {
          Vector2f v(e.xbutton.x, e.xbutton.y);
          auto input_event =
              std::make_unique<InputEvent>(InputEvent::kDragStart, 0, v);
          observer_->AddInputEvent(std::move(input_event));
        }
        break;
      }
      case ButtonRelease: {
        if (e.xbutton.button == 1) {
          Vector2f v(e.xbutton.x, e.xbutton.y);
          auto input_event =
              std::make_unique<InputEvent>(InputEvent::kDragEnd, 0, v);
          observer_->AddInputEvent(std::move(input_event));
        }
        break;
      }
      case FocusOut: {
        observer_->LostFocus();
        break;
      }
      case FocusIn: {
        observer_->GainedFocus(false);
        break;
      }
      case ClientMessage: {
        // WM_DELETE_WINDOW is the only registered type for now.
        should_exit_ = true;
        break;
      }
      case ConfigureNotify: {
        XConfigureEvent xce = e.xconfigure;
        observer_->OnWindowResized(xce.width, xce.height);
      }
    }
  }
}

void Platform::Exit() {
  should_exit_ = true;
}

void Platform::Vibrate(int duration) {}

void Platform::ShowInterstitialAd() {}

void Platform::ShareFile(const std::string& file_name) {}

void Platform::SetKeepScreenOn(bool keep_screen_on) {}

bool Platform::CreateWindow(int width, int height) {
  // Try to open the local display.
  display_ = XOpenDisplay(NULL);
  if (!display_) {
    LOG(0) << "Can't connect to X server. Try to set the DISPLAY environment "
              "variable (hostname:number.screen_number).";
    return false;
  }

  Window root_window = DefaultRootWindow(display_);

  XVisualInfo* visual_info = GetXVisualInfo(display_);
  if (!visual_info) {
    LOG(0) << "No appropriate visual found.";
    return false;
  }
  LOG(0) << "Visual " << (void*)visual_info->visualid << " selected";

  // Create the main window.
  XSetWindowAttributes window_attributes;
  window_attributes.colormap =
      XCreateColormap(display_, root_window, visual_info->visual, AllocNone);
  window_attributes.event_mask = ExposureMask | KeyPressMask;
  window_ = XCreateWindow(display_, root_window, 0, 0, width, height, 0,
                          visual_info->depth, InputOutput, visual_info->visual,
                          CWColormap | CWEventMask, &window_attributes);
  XMapWindow(display_, window_);
  XStoreName(display_, window_, "kaliber");

  return true;
}

void Platform::DestroyWindow() {
  if (display_) {
    XDestroyWindow(display_, window_);
#if 0  // TODO: Figure out why XCloseDisplay is crashing
    XCloseDisplay(display_);
#endif
    display_ = nullptr;
    window_ = 0;
  }
}

Display* Platform::GetDisplay() {
  return display_;
}

Window Platform::GetWindow() {
  return window_;
}

XVisualInfo* Platform::GetXVisualInfo(Display* display) {
  long visual_mask = VisualScreenMask;
  int num_visuals;
  XVisualInfo visual_info_template = {};
  visual_info_template.screen = DefaultScreen(display);
  return XGetVisualInfo(display, visual_mask, &visual_info_template,
                        &num_visuals);
}

}  // namespace eng

int main(int argc, char** argv) {
  eng::Platform platform;
  eng::KaliberMain(&platform);
  return 0;
}
