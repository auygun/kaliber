#if defined(__linux__)

#include "renderer.h"
#include <X11/Xutil.h>
#include "../../base/log.h"
#include "../../third_party/glew/glew.h"

namespace engine {

bool Renderer::SetupOpenGLWindow(int width, int height) {
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
  window = XCreateWindow(display, rootWindow, 0, 0, width, height, 0, visualInfo->depth,
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

  return true;
}

void Renderer::ShutdownOpenGLWindow() {
  if (display) {
    glXMakeCurrent(display, None, NULL);
    glXDestroyContext(display, glxContext);

    XDestroyWindow(display, window);
    XCloseDisplay(display);
  }
}

void Renderer::UpdateOpenGLWindow() {
  if (display)
    glXSwapBuffers(display, window);
}

void Renderer::PlatformInit(const std::set<std::string> &extensions) {
}

} // namespace engine

#endif
