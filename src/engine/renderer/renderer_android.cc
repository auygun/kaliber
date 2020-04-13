#include "renderer.h"
#include "../../base/log.h"
#include "../../third_party/android/GLContext.h"
#include <sstream>
#include <android/native_window.h>

namespace engine {

bool Renderer::Init(ANativeWindow* window) {
  ndk_helper::GLContext* gl_context = ndk_helper::GLContext::GetInstance();

  if (!gl_context->IsInitialzed()) {
    gl_context->Init(window);
    // TODO: LoadResources();
  } else if(window != gl_context->GetANativeWindow()) {
    // Re-initialize ANativeWindow.
    // On some devices, ANativeWindow is re-created when the app is resumed
    assert(gl_context->GetANativeWindow());
    // TODO: UnloadResources();
    gl_context->Invalidate();
    gl_context->Init(window);
    // TODO: LoadResources();
  } else {
    // TODO:
    // initialize OpenGL ES and EGL
    // if (EGL_SUCCESS == gl_context->Resume(window)) {
    //   UnloadResources();
    //   LoadResources();
    // } else {
    //     assert(0);
    // }
  }

  Init(gl_context->GetScreenWidth(), gl_context->GetScreenHeight());

  return true;
}

bool Renderer::SetupOpenGLWindow(int width, int height) {
  return true;
}

void Renderer::ShutdownOpenGLWindow() {
  // TODO: gl_context_->Invalidate();
  ndk_helper::GLContext::GetInstance()->Suspend();
}

void Renderer::UpdateOpenGLWindow() {
  if (EGL_SUCCESS != ndk_helper::GLContext::GetInstance()->Swap()) {
    // TODO:
    // UnloadResources();
    // LoadResources();
  }
}

void Renderer::PlatformInit(const std::set<std::string> &extensions) {
  if (extensions.find("GL_OES_vertex_array_object") != extensions.end()) {
      LOG("Supports Vertex Array Objects\n");
      vertexArrayObjects = true;
  }
}

} // namespace engine
