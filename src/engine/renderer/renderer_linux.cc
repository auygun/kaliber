#include "renderer.h"

#include "../../base/log.h"

namespace eng {

bool Renderer::Initialize() {
  LOG << "Initializing renderer.";

  if (!CreateWindow())
    return false;
  return StartRenderThread();
}

void Renderer::Shutdown() {
  LOG << "Shutting down renderer.";

  TerminateRenderThread();
  DestroyWindow();
}

bool Renderer::CreateWindow() {
  screen_width_ = 1280;
  screen_height_ = 1024;

  // Try to open the local display.
  display_ = XOpenDisplay(NULL);
  if (!display_) {
    LOG << "Can't connect to X server. Try to set the DISPLAY environment "
           "variable (hostname:number.screen_number).";
    return false;
  }

  Window root_window = DefaultRootWindow(display_);

  // Look for the right visual to set up the OpenGL context.
  GLint glx_attributes[] = {GLX_RGBA, GLX_DEPTH_SIZE, 24, GLX_DOUBLEBUFFER,
                            None};
  visual_info_ = glXChooseVisual(display_, 0, glx_attributes);
  if (!visual_info_) {
    LOG << "No appropriate visual found.";
    return false;
  }
  LOG << "Visual " << (void*)visual_info_->visualid << " selected";

  // Create the main window.
  XSetWindowAttributes window_attributes;
  window_attributes.colormap =
      XCreateColormap(display_, root_window, visual_info_->visual, AllocNone);
  window_attributes.event_mask = ExposureMask | KeyPressMask;
  window_ =
      XCreateWindow(display_, root_window, 0, 0, screen_width_, screen_height_,
                    0, visual_info_->depth, InputOutput, visual_info_->visual,
                    CWColormap | CWEventMask, &window_attributes);
  XMapWindow(display_, window_);
  XStoreName(display_, window_, "kaliber");

  return true;
}

bool Renderer::InitInternal() {
  // Create the OpenGL context.
  glx_context_ = glXCreateContext(display_, visual_info_, NULL, GL_TRUE);
  if (!glx_context_) {
    LOG << "Couldn't create the glx context.";
    return false;
  }

  glXMakeCurrent(display_, window_, glx_context_);

  if (GLEW_OK != glewInit()) {
    LOG << "Couldn't initialize OpenGL extension wrangler.";
    return false;
  }

  return InitCommon();
}

void Renderer::DestroyWindow() {
  if (display_) {
    XDestroyWindow(display_, window_);
    XCloseDisplay(display_);
  }
}

void Renderer::ShutdownInternal() {
  if (display_ && glx_context_) {
    glXMakeCurrent(display_, None, NULL);
    glXDestroyContext(display_, glx_context_);
    glx_context_ = nullptr;
  }
}

void Renderer::HandleCmdPresent(RenderCommand* cmd) {
  if (display_) {
    glXSwapBuffers(display_, window_);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  }
}

}  // namespace eng
