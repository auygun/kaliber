#include "engine/renderer/vulkan/renderer_vulkan.h"

#include "base/log.h"
#include "engine/platform/platform.h"
#include "third_party/glfw/glfw/include/GLFW/glfw3.h"

namespace eng {

bool RendererVulkan::Initialize(Platform* platform) {
  LOG(0) << "Initializing renderer.";

  // Use the framebuffer size, not the window size. They differ on HiDPI and
  // on Wayland with fractional/integer output scaling, and the swapchain has
  // to match the framebuffer.
  int width, height;
  glfwGetFramebufferSize(platform->GetWindow(), &width, &height);

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
