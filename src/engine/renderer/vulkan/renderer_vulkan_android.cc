#include "engine/renderer/vulkan/renderer_vulkan.h"

#include <android/native_window.h>

#include "base/log.h"

namespace eng {

bool RendererVulkan::Initialize(ANativeWindow* window) {
  LOG << "Initializing renderer.";

  screen_width_ = ANativeWindow_getWidth(window);
  screen_height_ = ANativeWindow_getHeight(window);

  if (!context_->CreateWindow(window, screen_width_, screen_height_)) {
    LOG << "Vulkan context failed to create window.";
    return false;
  }

  return InitializeInternal();
}

}  // namespace eng
