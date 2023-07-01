#include "engine/renderer/opengl/renderer_opengl.h"

#include "base/log.h"
#include "engine/platform/platform.h"

namespace eng {

bool RendererOpenGL::Initialize(Platform* platform) {
  LOG(0) << "Initializing renderer.";

  display_ = platform->GetDisplay();
  window_ = platform->GetWindow();

  GLint glx_attributes[] = {GLX_RGBA, GLX_DEPTH_SIZE, 24, GLX_DOUBLEBUFFER,
                            None};
  XVisualInfo* visual_info = glXChooseVisual(display_, 0, glx_attributes);
  glx_context_ = glXCreateContext(display_, visual_info, NULL, GL_TRUE);
  if (!glx_context_) {
    LOG(0) << "Couldn't create the glx context.";
    return false;
  }

  glXMakeCurrent(display_, window_, glx_context_);

  if (GLEW_OK != glewInit()) {
    LOG(0) << "Couldn't initialize OpenGL extension wrangler.";
    return false;
  }

  XWindowAttributes xwa;
  XGetWindowAttributes(display_, window_, &xwa);
  OnWindowResized(xwa.width, xwa.height);

  return InitCommon();
}

void RendererOpenGL::OnDestroy() {}

void RendererOpenGL::ShutdownInternal() {
  if (display_ && glx_context_) {
    glXMakeCurrent(display_, None, NULL);
    glXDestroyContext(display_, glx_context_);
    glx_context_ = nullptr;
  }
}

void RendererOpenGL::Present() {
  glXSwapBuffers(display_, window_);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  active_shader_id_ = 0;
  active_texture_id_ = 0;
  fps_++;
}

}  // namespace eng
