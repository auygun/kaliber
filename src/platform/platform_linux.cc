#include "../base/log.h"
#include "../engine/engine.h"
#include "../engine/renderer/renderer.h"
#include "platform.h"
#include <X11/Xlib.h>
// #include <pthread.h>

// void PTreadWorkaround() {
//   int i = pthread_getconcurrency();
// };

void Platform::Initialize() {
  root_path_ = "../../assets/";
  LOG("Root path: %s\n", root_path_.c_str());
  if (!engine::Engine::Get().GetRenderer().Init()) {
    LOG("Failed to initialize the renderer.\n");
    throw internal_error;
  }

  Display* display = engine::Engine::Get().GetRenderer().display();
  Window window = engine::Engine::Get().GetRenderer().window();
  Atom WM_DELETE_WINDOW = XInternAtom(display, "WM_DELETE_WINDOW", false);
  XSetWMProtocols(display, window, &WM_DELETE_WINDOW, 1);
}

void Platform::Update() {
  Display* display = engine::Engine::Get().GetRenderer().display();
  if (!XPending(display))
    return;
  XEvent e;
  XNextEvent(display, &e);
  if (e.type == KeyPress) {
    if (e.xkey.keycode == XKeysymToKeycode(display, XK_Y) &&
        !(e.xkey.state & (ShiftMask | ControlMask | Mod1Mask | Mod4Mask)))
      LOG("Y pressed!!! %d\n", e.xkey.state);
  } else if (e.type == ClientMessage) {
    // TODO: Should check here for other client message types. However the only
    // protocol registered above is WM_DELETE_WINDOW for now.
    should_exit_ = true;
  }
}

int main(int argc, char** argv) {
  // PTreadWorkaround();
  Platform& platform = Platform::Get();
  try {
    platform.Initialize();
    platform.RunMainLoop();
    platform.Shutdown();
  } catch (Platform::InternalError& e) {
    return -1;
  }
  return 0;
}
