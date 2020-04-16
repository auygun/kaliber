#include "renderer.h"
#include "../../base/log.h"
#include "../../platform/platform.h"
#include "../../third_party/android/GLContext.h"
#include <cassert>
#include <sstream>
#include <android/native_window.h>

namespace engine {

bool Renderer::Init() {
  ndk_helper::GLContext* gl_context = ndk_helper::GLContext::GetInstance();
  ANativeWindow *window = Platform::Get().GetNativeWindow();

  if (!gl_context->IsInitialzed()) {
    gl_context->Init(window);
    // TODO: LoadResources();
  } else if(window != gl_context->GetANativeWindow()) {
    // Re-initialize ANativeWindow.
    // On some devices, ANativeWindow is re-created when the app is resumed
    ContextLost();
    gl_context->Invalidate();
    gl_context->Init(window);
    // TODO: LoadResources();
  } else {
    // initialize OpenGL ES and EGL
    if (EGL_SUCCESS == gl_context->Resume(window)) {
      ContextLost();
      // TODO: LoadResources();
    } else {
        return false;
    }
  }

  screen_width_ = gl_context->GetScreenWidth();
  screen_height_ = gl_context->GetScreenHeight();

  LogVersion();
  LOG("Screen size: %d, %d\n", screen_width_, screen_height_);

  std::set<std::string> extensions = SetupExtensions();

  if (extensions.find("GL_OES_vertex_array_object") != extensions.end()) {
      LOG("Supports Vertex Array Objects\n");
      vertexArrayObjects = true;
  }

  glViewport(0, 0, screen_width_, screen_height_);
  return true;
}

void Renderer::Shutdown() {
  ndk_helper::GLContext::GetInstance()->Suspend();
}

void Renderer::Present() {
  if (EGL_SUCCESS != ndk_helper::GLContext::GetInstance()->Swap()) {
    // TODO:
    // UnloadResources();
    // LoadResources();
  }
}

void Renderer::TrimMemory() {
  LOG("Trimming memor\n");
  ndk_helper::GLContext::GetInstance()->Invalidate();
}

} // namespace engine
