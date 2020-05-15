#include <android/native_window.h>
#include <cassert>
#include <sstream>
#include "../../base/log.h"
#include "../../third_party/android/GLContext.h"
#include "renderer.h"

namespace eng {

bool Renderer::Init(ANativeWindow* window) {
  window_ = window;
  return StartWorker();
}

bool Renderer::Init() {
  // Unreachable code.
  assert(false);
}

void Renderer::Shutdown() {
  TerminateWorker();
}

bool Renderer::InitInternal() {
  ndk_helper::GLContext* gl_context = ndk_helper::GLContext::GetInstance();

  if (!gl_context->IsInitialzed()) {
    gl_context->Init(window_);
    // TODO: LoadResources();
  } else if (window_ != gl_context->GetANativeWindow()) {
    // Re-initialize ANativeWindow.
    // On some devices, ANativeWindow is re-created when the app is resumed
    gl_context->Invalidate();
    gl_context->Init(window_);
    ContextLost();
    // TODO: LoadResources();
  } else {
    // initialize OpenGL ES and EGL
    if (EGL_SUCCESS == gl_context->Resume(window_)) {
      ContextLost();
      // TODO: LoadResources();
    } else {
      return false;
    }
  }

  screen_width_ = gl_context->GetScreenWidth();
  screen_height_ = gl_context->GetScreenHeight();

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

void Renderer::ShutdownInternal() {
  ndk_helper::GLContext::GetInstance()->Suspend();
}

void Renderer::HandleCmdPresent(RenderCommand* cmd) {
  if (EGL_SUCCESS != ndk_helper::GLContext::GetInstance()->Swap()) {
    // TODO:
    // UnloadResources();
    // LoadResources();
    ContextLost();
  }
}

void Renderer::TrimMemory() {
  LOG << "Trimming memor";
  ndk_helper::GLContext::GetInstance()->Invalidate();
}

}  // namespace eng
