#include "engine/renderer/opengl/renderer_opengl.h"

#include "base/log.h"
#include "engine/platform/platform.h"

namespace eng {

bool RendererOpenGL::Initialize(Platform* platform) {
  LOG(0) << "Initializing renderer.";

  wnd_ = platform->GetWindow();
  dc_ = GetDC(wnd_);

  PIXELFORMATDESCRIPTOR pfd = {
      sizeof(PIXELFORMATDESCRIPTOR), 1,
      PFD_DRAW_TO_WINDOW |      // format must support window
          PFD_SUPPORT_OPENGL |  // format must support OpenGL
          PFD_DOUBLEBUFFER,
      PFD_TYPE_RGBA,
      32,  // color depth
      // Color bits ignored
      0, 0, 0, 0, 0, 0,
      0,  // alpha buffer
      0,  // shift bit ignored
      0,  // no accumulation buffer
      // Accumulation bits ignored
      0, 0, 0, 0,
      24,  // z-buffer
      0,   // no stencil buffer
      0,   // no auxiliary buffer
      PFD_MAIN_PLANE,
      0,  // reserved
      // layer masks ignored
      0, 0, 0};
  int pf = ChoosePixelFormat(dc_, &pfd);
  if (pf == 0) {
    LOG(0) << "ChoosePixelFormat failed.";
    return false;
  }
  SetPixelFormat(dc_, pf, &pfd);

  glrc_ = wglCreateContext(dc_);
  wglMakeCurrent(dc_, glrc_);

  if (GLEW_OK != glewInit()) {
    LOG(0) << "Couldn't initialize OpenGL extension wrangler.";
    return false;
  }

  RECT rect;
  GetClientRect(platform->GetWindow(), &rect);
  OnWindowResized(rect.right, rect.bottom);

  return InitCommon();
}

void RendererOpenGL::OnDestroy() {}

void RendererOpenGL::Shutdown() {
  LOG(0) << "Shutting down renderer.";
  is_initialized_ = false;
  if (dc_ && glrc_) {
    wglMakeCurrent(nullptr, nullptr);
    wglDeleteContext(glrc_);
    ReleaseDC(wnd_, dc_);
    dc_ = nullptr;
    glrc_ = nullptr;
  }
}

void RendererOpenGL::Present() {
  SwapBuffers(dc_);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  active_shader_id_ = 0;
  active_texture_id_ = 0;
  fps_++;
}

}  // namespace eng
