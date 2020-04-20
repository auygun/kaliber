#if defined(__linux__)

#include "renderer.h"
#include <X11/Xutil.h>
#include "../../base/log.h"
#include "../../third_party/glew/glew.h"

namespace engine {

bool Renderer::Init() {
  screen_width_ = 1280;
  screen_height_ = 1024;

  // Try to open the local display.
  display = XOpenDisplay(NULL);
  if (!display) {
    LOG("Can't connect to X server. Try to set the DISPLAY environment variable (hostname:number.screen_number).\n");
    return false;
  }

  Window rootWindow = DefaultRootWindow(display);

  // Look for the right visual to set up the OpenGL context.
  GLint glxAttributes[] = { GLX_RGBA, GLX_DEPTH_SIZE, 24, GLX_DOUBLEBUFFER, None };
  XVisualInfo *visualInfo = glXChooseVisual(display, 0, glxAttributes);
  if (!visualInfo) {
    LOG("No appropriate visual found.\n");
    return false;
  }
  LOG("Visual %p selected\n", (void *)visualInfo->visualid);

  // Create the main window.
  XSetWindowAttributes windowAttributes;
  windowAttributes.colormap = XCreateColormap(display, rootWindow, visualInfo->visual, AllocNone);
  windowAttributes.event_mask = ExposureMask | KeyPressMask;
  window = XCreateWindow(display, rootWindow, 0, 0, screen_width_, screen_height_, 0, visualInfo->depth,
                         InputOutput, visualInfo->visual, CWColormap | CWEventMask, &windowAttributes);
  XMapWindow(display, window);
  XStoreName(display, window, "Opera Testbed");

  // Create the OpenGL context.
  glxContext = glXCreateContext(display, visualInfo, NULL, GL_TRUE);
  if (!glxContext) {
    LOG("Couldn't create the glx context.\n");
    return false;
  }

  glXMakeCurrent(display, window, glxContext);

  if (GLEW_OK != glewInit()) {
    LOG("Couldn't initialize OpenGL extension wrangler.\n");
    return false;
  }

  LogVersion();
  LOG("Screen size: %d, %d\n", screen_width_, screen_height_);

  std::set<std::string> extensions = SetupExtensions();

  if (extensions.find("GL_OES_vertex_array_object") != extensions.end()) {
      LOG("Supports Vertex Array Objects\n");
      vertexArrayObjects = true;
  }

  glViewport(0, 0, screen_width_, screen_height_);

  Atom WM_DELETE_WINDOW = XInternAtom(display, "WM_DELETE_WINDOW", false);
  XSetWMProtocols(display, window, &WM_DELETE_WINDOW, 1);

  return true;
}

bool Renderer::ShouldExit() {
  if (!XPending(display))
    return false;
  XEvent e;
  XNextEvent(display, &e);
  if (e.type == KeyPress) {
    return false;
  } else if (e.type == ClientMessage) {
    // TODO: Should check here for other client message types. However the only
    // protocol registered above is WM_DELETE_WINDOW for now.
    return true;
  }
  return false;
}

void Renderer::Shutdown() {
  if (display) {
    glXMakeCurrent(display, None, NULL);
    glXDestroyContext(display, glxContext);

    XDestroyWindow(display, window);
    XCloseDisplay(display);
  }
}

void Renderer::Present() {
  if (display)
    glXSwapBuffers(display, window);
}

void Renderer::TrimMemory() {}

} // namespace engine

#endif
