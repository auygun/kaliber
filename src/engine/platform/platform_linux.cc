#include "platform_linux.h"

#include <X11/Xlib.h>
#include <X11/Xutil.h>

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

PlatformLinux::PlatformLinux() = default;
PlatformLinux::~PlatformLinux() = default;

void PlatformLinux::Initialize() {
  PlatformBase::Initialize();

  root_path_ = "../../";
  LOG << "Root path: " << root_path_.c_str();

  if (!renderer_->Initialize()) {
    LOG << "Failed to initialize renderer.";
    throw internal_error;
  }

  Display* display = renderer_->display();
  Window window = renderer_->window();
  XSelectInput(display, window,
               KeyPressMask | Button1MotionMask | ButtonPressMask |
                   ButtonReleaseMask | FocusChangeMask);
  Atom WM_DELETE_WINDOW = XInternAtom(display, "WM_DELETE_WINDOW", false);
  XSetWMProtocols(display, window, &WM_DELETE_WINDOW, 1);
}

void PlatformLinux::Update() {
  Display* display = renderer_->display();
  while (XPending(display)) {
    XEvent e;
    XNextEvent(display, &e);
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
        Vector2 v(e.xmotion.x, e.xmotion.y);
        v = engine_->ToPosition(v);
        // DLOG << "drag: " << v;
        auto input_event = std::make_unique<InputEvent>(InputEvent::kDrag, 0,
                                                        v * Vector2(1, -1));
        engine_->AddInputEvent(std::move(input_event));
        break;
      }
      case ButtonPress: {
        if (e.xbutton.button == 1) {
          Vector2 v(e.xbutton.x, e.xbutton.y);
          v = engine_->ToPosition(v);
          // DLOG << "drag-start: " << v;
          auto input_event = std::make_unique<InputEvent>(
              InputEvent::kDragStart, 0, v * Vector2(1, -1));
          engine_->AddInputEvent(std::move(input_event));
        }
        break;
      }
      case ButtonRelease: {
        if (e.xbutton.button == 1) {
          Vector2 v(e.xbutton.x, e.xbutton.y);
          v = engine_->ToPosition(v);
          // DLOG << "drag-end!";
          auto input_event = std::make_unique<InputEvent>(
              InputEvent::kDragEnd, 0, v * Vector2(1, -1));
          engine_->AddInputEvent(std::move(input_event));
        }
        break;
      }
      case FocusOut: {
        engine_->LostFocus();
        break;
      }
      case FocusIn: {
        engine_->GainedFocus();
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

void PlatformLinux::Exit() {
  should_exit_ = true;
}

}  // namespace eng

int main(int argc, char** argv) {
  eng::PlatformLinux platform;
  try {
    platform.Initialize();
    platform.RunMainLoop();
    platform.Shutdown();
  } catch (eng::PlatformBase::InternalError& e) {
    return -1;
  }
  return 0;
}
