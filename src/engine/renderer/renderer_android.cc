#include "renderer.h"

#include <android/native_window.h>

#include "../../base/log.h"
#include "../../third_party/android/GLContext.h"

namespace eng {

bool Renderer::Initialize(ANativeWindow* window) {
  LOG << "Initializing renderer.";

  window_ = window;
  return StartRenderThread();
}

void Renderer::Shutdown() {
  if (terminate_render_thread_)
    return;

  LOG << "Shutting down renderer.";

  TerminateRenderThread();
}

bool Renderer::InitInternal() {
  ndk_helper::GLContext* gl_context = ndk_helper::GLContext::GetInstance();

  if (!gl_context->IsInitialzed()) {
    gl_context->Init(window_);
  } else if (window_ != gl_context->GetANativeWindow()) {
    // Re-initialize ANativeWindow.
    // On some devices, ANativeWindow is re-created when the app is resumed
    gl_context->Invalidate();
    gl_context->Init(window_);
    ContextLost();
  } else {
    // initialize OpenGL ES and EGL
    if (EGL_SUCCESS == gl_context->Resume(window_)) {
      ContextLost();
    } else {
      return false;
    }
  }

  screen_width_ = gl_context->GetScreenWidth();
  screen_height_ = gl_context->GetScreenHeight();

  if (gl_context->GetGLVersion() >= 3)
    npot_ = true;

  return InitCommon();
}

void Renderer::ShutdownInternal() {
  ndk_helper::GLContext::GetInstance()->Suspend();
}

void Renderer::HandleCmdPresent(RenderCommand* cmd) {
  if (EGL_SUCCESS != ndk_helper::GLContext::GetInstance()->Swap()) {
    ContextLost();
    return;
  }
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

}  // namespace eng
