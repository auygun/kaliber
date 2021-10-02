#include "platform.h"

#include <memory>

#include "../../base/log.h"
#include "../../base/task_runner.h"
#include "../../base/vecmath.h"
#include "../audio/audio.h"
#include "../engine.h"
#include "../input_event.h"
#include "../renderer/renderer.h"

using namespace base;

namespace eng {

void Platform::Initialize() {
  Platform::InitializeCommon();

  root_path_ = "../../";
  LOG << "Root path: " << root_path_.c_str();

  data_path_ = "./";
  LOG << "Data path: " << data_path_.c_str();

  shared_data_path_ = "./";
  LOG << "Shared data path: " << shared_data_path_.c_str();

  if (!CreateWindow(800, 1205)) {
    LOG << "Failed to create window.";
    throw internal_error;
  }

  if (!renderer_->Initialize(display_, window_)) {
    LOG << "Failed to initialize renderer.";
    throw internal_error;
  }

  XSelectInput(display_, window_,
               KeyPressMask | Button1MotionMask | ButtonPressMask |
                   ButtonReleaseMask | FocusChangeMask);
  Atom WM_DELETE_WINDOW = XInternAtom(display_, "WM_DELETE_WINDOW", false);
  XSetWMProtocols(display_, window_, &WM_DELETE_WINDOW, 1);
}

void Platform::Shutdown() {
  Platform::ShutdownCommon();

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
        engine_->AddInputEvent(std::move(input_event));
        // TODO: e.xkey.state & (ShiftMask | ControlMask | Mod1Mask | Mod4Mask))
        break;
      }
      case MotionNotify: {
        Vector2f v(e.xmotion.x, e.xmotion.y);
        v = engine_->ToPosition(v);
        // DLOG << "drag: " << v;
        auto input_event = std::make_unique<InputEvent>(InputEvent::kDrag, 0,
                                                        v * Vector2f(1, -1));
        engine_->AddInputEvent(std::move(input_event));
        break;
      }
      case ButtonPress: {
        if (e.xbutton.button == 1) {
          Vector2f v(e.xbutton.x, e.xbutton.y);
          v = engine_->ToPosition(v);
          // DLOG << "drag-start: " << v;
          auto input_event = std::make_unique<InputEvent>(
              InputEvent::kDragStart, 0, v * Vector2f(1, -1));
          engine_->AddInputEvent(std::move(input_event));
        }
        break;
      }
      case ButtonRelease: {
        if (e.xbutton.button == 1) {
          Vector2f v(e.xbutton.x, e.xbutton.y);
          v = engine_->ToPosition(v);
          // DLOG << "drag-end!";
          auto input_event = std::make_unique<InputEvent>(
              InputEvent::kDragEnd, 0, v * Vector2f(1, -1));
          engine_->AddInputEvent(std::move(input_event));
        }
        break;
      }
      case FocusOut: {
        engine_->LostFocus();
        break;
      }
      case FocusIn: {
        engine_->GainedFocus(false);
        break;
      }
      case ClientMessage: {
        // WM_DELETE_WINDOW is the only registered type for now.
        should_exit_ = true;
        break;
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
    LOG << "Can't connect to X server. Try to set the DISPLAY environment "
           "variable (hostname:number.screen_number).";
    return false;
  }

  Window root_window = DefaultRootWindow(display_);

  XVisualInfo* visual_info = renderer_->GetXVisualInfo(display_);
  if (!visual_info) {
    LOG << "No appropriate visual found.";
    return false;
  }
  LOG << "Visual " << (void*)visual_info->visualid << " selected";

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
    XCloseDisplay(display_);
    display_ = nullptr;
    window_ = 0;
  }
}

}  // namespace eng

int main(int argc, char** argv) {
  eng::Platform platform;
  try {
    platform.Initialize();
    platform.RunMainLoop();
    platform.Shutdown();
  } catch (eng::Platform::InternalError& e) {
    return -1;
  }
  return 0;
}
