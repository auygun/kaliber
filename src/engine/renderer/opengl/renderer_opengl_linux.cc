#include "engine/renderer/opengl/renderer_opengl.h"

#include "base/log.h"

namespace eng {

bool RendererOpenGL::Initialize(Display* display, Window window) {
  LOG << "Initializing renderer.";

  display_ = display;
  window_ = window;

  XWindowAttributes xwa;
  XGetWindowAttributes(display_, window_, &xwa);
  screen_width_ = xwa.width;
  screen_height_ = xwa.height;

  return StartRenderThread();
}

void RendererOpenGL::OnDestroy() {}

bool RendererOpenGL::InitInternal() {
  // Create the OpenGL context.
  glx_context_ =
      glXCreateContext(display_, GetXVisualInfo(display_), NULL, GL_TRUE);
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

void RendererOpenGL::ShutdownInternal() {
  if (display_ && glx_context_) {
    glXMakeCurrent(display_, None, NULL);
    glXDestroyContext(display_, glx_context_);
    glx_context_ = nullptr;
  }
}

void RendererOpenGL::HandleCmdPresent(RenderCommand* cmd) {
  if (display_) {
    glXSwapBuffers(display_, window_);
#ifdef THREADED_RENDERING
    draw_complete_semaphore_.release();
#endif  // THREADED_RENDERING
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    active_shader_id_ = 0;
    active_texture_id_ = 0;
  }
}

XVisualInfo* RendererOpenGL::GetXVisualInfo(Display* display) {
  // Look for the right visual to set up the OpenGL context.
  GLint glx_attributes[] = {GLX_RGBA, GLX_DEPTH_SIZE, 24, GLX_DOUBLEBUFFER,
                            None};
  return glXChooseVisual(display, 0, glx_attributes);
}

}  // namespace eng
