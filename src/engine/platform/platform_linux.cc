#include "platform.h"

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include "../../base/log.h"
#include "../../base/vecmath.h"
#include "../engine.h"
#include "../input_event.h"
#include "../renderer/renderer.h"

using base::Vector2;

namespace eng {

Platform::Platform() = default;
Platform::~Platform() = default;

void Platform::Initialize() {
  root_path_ = "../../";
  LOG << "Root path: " << root_path_.c_str();

  renderer_ = std::make_unique<Renderer>();
  if (!renderer_->Init()) {
    LOG << "Failed to initialize renderer.";
    throw internal_error;
  }
  LOG << "Initialized the renderer.";

  Display* display = renderer_->display();
  Window window = renderer_->window();
  XSelectInput(
      display, window,
      KeyPressMask | Button1MotionMask | ButtonPressMask | ButtonReleaseMask);
  Atom WM_DELETE_WINDOW = XInternAtom(display, "WM_DELETE_WINDOW", false);
  XSetWMProtocols(display, window, &WM_DELETE_WINDOW, 1);
}

void Platform::Update() {
  Display* display = renderer_->display();
  while (XPending(display)) {
    XEvent e;
    XNextEvent(display, &e);
    switch (e.type) {
      case KeyPress: {
        KeySym key = XLookupKeysym(&e.xkey, 0);
        auto input_event =
            std::make_unique<InputEvent>(InputEvent::kKeyPress, key);
        Engine::Get().AddInputEvent(std::move(input_event));
        // TODO: e.xkey.state & (ShiftMask | ControlMask | Mod1Mask | Mod4Mask))
        break;
      }
      case MotionNotify: {
        Vector2 v(e.xmotion.x, e.xmotion.y);
        v = Engine::Get().ToPosition(v);
        // DLOG << "drag: " << v;
        auto input_event =
            std::make_unique<InputEvent>(InputEvent::kDrag, v * Vector2(1, -1));
        Engine::Get().AddInputEvent(std::move(input_event));
        break;
      }
      case ButtonPress: {
        if (e.xbutton.button == 1) {
          Vector2 v(e.xbutton.x, e.xbutton.y);
          v = Engine::Get().ToPosition(v);
          // DLOG << "drag-start: " << v;
          auto input_event = std::make_unique<InputEvent>(
              InputEvent::kDragStart, v * Vector2(1, -1));
          Engine::Get().AddInputEvent(std::move(input_event));
        }
        break;
      }
      case ButtonRelease: {
        if (e.xbutton.button == 1) {
          // DLOG << "drag-end!";
          auto input_event = std::make_unique<InputEvent>(InputEvent::kDragEnd);
          Engine::Get().AddInputEvent(std::move(input_event));
        }
        break;
      }
      case ClientMessage: {
        // TODO: Should check here for other client message types. However the
        // only protocol registered above is WM_DELETE_WINDOW for now.
        should_exit_ = true;
        break;
      }
    }
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
