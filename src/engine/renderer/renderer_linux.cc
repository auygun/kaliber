#if defined(__linux__)

#include <X11/Xutil.h>
#include "../../base/log.h"
#include "../../third_party/glew/glew.h"
#include "renderer.h"

namespace engine {

bool Renderer::Init() {
  screen_width_ = 1280;
  screen_height_ = 1024;

  // Try to open the local display.
  display_ = XOpenDisplay(NULL);
  if (!display_) {
    LOG("Can't connect to X server. Try to set the DISPLAY environment "
        "variable (hostname:number.screen_number).\n");
    return false;
  }

  Window root_window = DefaultRootWindow(display_);

  // Look for the right visual to set up the OpenGL context.
  GLint glx_attributes[] = {GLX_RGBA, GLX_DEPTH_SIZE, 24, GLX_DOUBLEBUFFER,
                            None};
  XVisualInfo* visual_info = glXChooseVisual(display_, 0, glx_attributes);
  if (!visual_info) {
    LOG("No appropriate visual found.\n");
    return false;
  }
  LOG("Visual %p selected\n", (void*)visual_info->visualid);

  // Create the main window.
  XSetWindowAttributes window_attributes;
  window_attributes.colormap =
      XCreateColormap(display_, root_window, visual_info->visual, AllocNone);
  window_attributes.event_mask = ExposureMask | KeyPressMask;
  window_ =
      XCreateWindow(display_, root_window, 0, 0, screen_width_, screen_height_,
                    0, visual_info->depth, InputOutput, visual_info->visual,
                    CWColormap | CWEventMask, &window_attributes);
  XMapWindow(display_, window_);
  XStoreName(display_, window_, "Opera Testbed");

  // Create the OpenGL context.
  glx_context_ = glXCreateContext(display_, visual_info, NULL, GL_TRUE);
  if (!glx_context_) {
    LOG("Couldn't create the glx context.\n");
    return false;
  }

  glXMakeCurrent(display_, window_, glx_context_);

  if (GLEW_OK != glewInit()) {
    LOG("Couldn't initialize OpenGL extension wrangler.\n");
    return false;
  }

  LogVersion();
  LOG("Screen size: %d, %d\n", screen_width_, screen_height_);

  std::set<std::string> extensions = SetupExtensions();

  if (extensions.find("GL_OES_vertex_array_object") != extensions.end()) {
    LOG("Supports Vertex Array Objects\n");
    vertex_array_objects_ = true;
  }

  glViewport(0, 0, screen_width_, screen_height_);

  Atom WM_DELETE_WINDOW = XInternAtom(display_, "WM_DELETE_WINDOW", false);
  XSetWMProtocols(display_, window_, &WM_DELETE_WINDOW, 1);

  return true;
}

bool Renderer::ShouldExit() {
  if (!XPending(display_))
    return false;
  XEvent e;
  XNextEvent(display_, &e);
  if (e.type == KeyPress) {
    if (e.xkey.keycode == XKeysymToKeycode(display_, XK_Y) &&
        !(e.xkey.state & (ShiftMask | ControlMask | Mod1Mask | Mod4Mask)))
      LOG("Y pressed!!! %d\n", e.xkey.state);
    return false;
  } else if (e.type == ClientMessage) {
    // TODO: Should check here for other client message types. However the only
    // protocol registered above is WM_DELETE_WINDOW for now.
    return true;
  }
  return false;
}

void Renderer::Shutdown() {
  if (display_) {
    glXMakeCurrent(display_, None, NULL);
    glXDestroyContext(display_, glx_context_);

    XDestroyWindow(display_, window_);
    XCloseDisplay(display_);
  }
}

void Renderer::Present() {
  if (display_)
    glXSwapBuffers(display_, window_);
}

void Renderer::TrimMemory() {}

}  // namespace engine

#endif
