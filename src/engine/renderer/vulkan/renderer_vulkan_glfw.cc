#include "engine/renderer/vulkan/renderer_vulkan.h"

#include "base/log.h"
#include "engine/platform/platform.h"

namespace eng {

bool RendererVulkan::Initialize(Platform* platform) {
  LOG(0) << "Initializing renderer.";

  if (!glfwVulkanSupported()) {
    LOG(0) << "Vulkan not supported!";
    return false;
  }

  int width, height;
  glfwGetWindowSize(platform->GetWindow(), &width, &height);

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
