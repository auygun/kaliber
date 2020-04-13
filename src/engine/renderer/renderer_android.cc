#include "renderer.h"

#include <android/native_window.h>

#include "../../third_party/android/GLContext.h"

namespace eng {

bool Renderer::Initialize(ANativeWindow* window) {
  window_ = window;
  return StartWorker();
}

void Renderer::Shutdown() {
  TerminateWorker();
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
  }
}

}  // namespace eng
