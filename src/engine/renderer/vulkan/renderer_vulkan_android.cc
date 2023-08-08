#include "engine/renderer/vulkan/renderer_vulkan.h"

#include <android/native_window.h>

#include "base/log.h"
#include "engine/platform/platform.h"

namespace eng {

bool RendererVulkan::Initialize(Platform* platform) {
  LOG(0) << "Initializing renderer.";

  int width = ANativeWindow_getWidth(platform->GetWindow());
  int height = ANativeWindow_getHeight(platform->GetWindow());

  if (!context_.Initialize()) {
    LOG(0) << "Failed to initialize Vulkan context.";
    return false;
  }
  if (!context_.CreateSurface(platform->GetWindow(), width, height)) {
    LOG(0) << "Vulkan context failed to create window.";
    return false;
  }

  return InitializeInternal();
}

}  // namespace eng
