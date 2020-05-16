#include "../../base/log.h"
#include "../engine.h"
#include "../input_event.h"
#include "../renderer/renderer.h"
#include "platform.h"
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include <pthread.h>

namespace eng {

Platform::Platform() = default;
Platform::~Platform() = default;

void Platform::Initialize() {
  root_path_ = "../../assets/";
  LOG << "Root path: " << root_path_.c_str();

  renderer_ = std::make_unique<Renderer>();
  if (!renderer_->Init()) {
    LOG << "Failed to initialize renderer.";
    throw internal_error;
  }
  LOG << "Initialized the renderer.";

  Display* display = renderer_->display();
  Window window = renderer_->window();
  Atom WM_DELETE_WINDOW = XInternAtom(display, "WM_DELETE_WINDOW", false);
  XSetWMProtocols(display, window, &WM_DELETE_WINDOW, 1);
}

void Platform::Update() {
  Display* display = renderer_->display();
  if (!XPending(display))
    return;
  XEvent e;
  XNextEvent(display, &e);
  if (e.type == KeyPress) {
    KeySym key = XLookupKeysym(&e.xkey, 0);
    auto input_event = std::make_unique<InputEvent>(InputEvent::kKeyPress, key);
    Engine::Get().AddInputEvent(std::move(input_event));
    // TODO: e.xkey.state & (ShiftMask | ControlMask | Mod1Mask | Mod4Mask))
  } else if (e.type == ClientMessage) {
    // TODO: Should check here for other client message types. However the only
    // protocol registered above is WM_DELETE_WINDOW for now.
    should_exit_ = true;
  }
}

} // namespace eng

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
