#include "engine/renderer/opengl/renderer_opengl.h"

#include <android/native_window.h>

#include "base/log.h"
#include "engine/platform/platform.h"
#include "third_party/android/GLContext.h"

namespace eng {

bool RendererOpenGL::Initialize(Platform* platform) {
  LOG(0) << "Initializing renderer.";

  window_ = platform->GetWindow();
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

void RendererOpenGL::OnDestroy() {
  ndk_helper::GLContext::GetInstance()->Invalidate();
}

void RendererOpenGL::ShutdownInternal() {
  ndk_helper::GLContext::GetInstance()->Suspend();
}

void RendererOpenGL::Present() {
  if (EGL_SUCCESS != ndk_helper::GLContext::GetInstance()->Swap()) {
    ContextLost();
    return;
  }
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  active_shader_id_ = 0;
  active_texture_id_ = 0;
  fps_++;
}

}  // namespace eng
