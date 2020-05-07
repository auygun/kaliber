#if defined(__linux__)

#include <X11/Xutil.h>
#include "../../base/log.h"
#include "../../third_party/glew/glew.h"
#include "renderer.h"

namespace eng {

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
  XStoreName(display_, window_, "gltest");

  return true;
}

bool Renderer::Init() {
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

  // TODO: Move toplatform independend function

  LogVersion();
  LOG << "Screen size: " << screen_width_ << ", " << screen_height_;

  std::unordered_set<std::string> extensions = SetupExtensions();

  if (extensions.find("GL_OES_vertex_array_object") != extensions.end()) {
    LOG << "Supports Vertex Array Objects";
    vertex_array_objects_ = true;
  }

  glViewport(0, 0, screen_width_, screen_height_);

  // The orthogonal viewport is (-1.0 .. 1.0) for the short edge of the screen.
  // It's calculated from aspect ratio for the long endge.
  if (screen_width_ > screen_height_) {
    float screen_ratio = (float)screen_width_ / (float)screen_height_;
    LOG << "screen_ratio: " << screen_ratio;
    projection_ = Ortho(-screen_ratio, screen_ratio, -1.0f, 1.0f);
  } else {
    float screen_ratio = (float)screen_height_ / (float)screen_width_;
    LOG << "screen_ratio: " << screen_ratio;
    projection_ = Ortho(-1.0, 1.0, -screen_ratio, screen_ratio);
  }

  return true;
}

void Renderer::DestroyWindow() {
  if (display_) {
    XDestroyWindow(display_, window_);
    XCloseDisplay(display_);
  }
}

void Renderer::Shutdown() {
  if (display_ && glx_context_) {
    glXMakeCurrent(display_, None, NULL);
    glXDestroyContext(display_, glx_context_);
    glx_context_ = nullptr;
  }
}

void Renderer::HandleCmdPresent(RenderCommand* cmd) {
  if (display_)
    glXSwapBuffers(display_, window_);
}

void Renderer::TrimMemory() {}

}  // namespace eng

#endif
