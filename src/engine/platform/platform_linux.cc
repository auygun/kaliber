#include "../../base/log.h"
#include "../engine.h"
#include "../input_event.h"
#include "../renderer/renderer.h"
#include "platform.h"
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include <pthread.h>

Platform::Platform() = default;
Platform::~Platform() = default;

void Platform::Initialize() {
  root_path_ = "../../assets/";
  LOG << "Root path: " << root_path_.c_str();

  if (!eng::Engine::Get().GetRenderer().Init()) {
    LOG << "Failed to initialize renderer.";
    throw internal_error;
  }
  LOG << "Initialized the renderer.";

  Display* display = eng::Engine::Get().GetRenderer().display();
  Window window = eng::Engine::Get().GetRenderer().window();
  Atom WM_DELETE_WINDOW = XInternAtom(display, "WM_DELETE_WINDOW", false);
  XSetWMProtocols(display, window, &WM_DELETE_WINDOW, 1);
}

void Platform::Update() {
  Display* display = eng::Engine::Get().GetRenderer().display();
  if (!XPending(display))
    return;
  XEvent e;
  XNextEvent(display, &e);
  if (e.type == KeyPress) {
    KeySym key = XLookupKeysym(&e.xkey, 0);
    auto input_event = std::make_unique<eng::InputEvent>(
        eng::InputEvent::kKeyPress, key);
    eng::Engine::Get().AddInputEvent(std::move(input_event));
    // TODO: e.xkey.state & (ShiftMask | ControlMask | Mod1Mask | Mod4Mask))
  } else if (e.type == ClientMessage) {
    // TODO: Should check here for other client message types. However the only
    // protocol registered above is WM_DELETE_WINDOW for now.
    should_exit_ = true;
  }
}

int main(int argc, char** argv) {
  Platform platform;
  try {
    platform.Initialize();
    platform.RunMainLoop();
    platform.Shutdown();
  } catch (Platform::InternalError& e) {
    return -1;
  }
  return 0;
}
