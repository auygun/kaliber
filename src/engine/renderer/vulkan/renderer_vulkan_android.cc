#include "engine/renderer/vulkan/renderer_vulkan.h"

#include <android/native_window.h>

#include "base/log.h"
#include "engine/platform/platform.h"

namespace eng {

bool RendererVulkan::Initialize(Platform* platform) {
  LOG << "Initializing renderer.";

  screen_width_ = ANativeWindow_getWidth(platform->GetWindow());
  screen_height_ = ANativeWindow_getHeight(platform->GetWindow());

  if (!context_.Initialize()) {
    LOG << "Failed to initialize Vulkan context.";
    return false;
  }
  if (!context_.CreateWindow(platform->GetWindow(), screen_width_,
                             screen_height_)) {
    LOG << "Vulkan context failed to create window.";
    return false;
  }

  return InitializeInternal();
}

}  // namespace eng
