#include "engine/renderer/vulkan/vulkan_context.h"

#include <cstring>

#include "base/log.h"
#include "third_party/glfw/glfw/include/GLFW/glfw3.h"

namespace eng {

const char* VulkanContext::GetPlatformSurfaceExtension() const {
  uint32_t count = 0;
  const char** extensions = glfwGetRequiredInstanceExtensions(&count);
  for (uint32_t i = 0; i < count; i++) {
    if (strcmp(extensions[i], VK_KHR_SURFACE_EXTENSION_NAME) != 0)
      return extensions[i];
  }
  return "";
}

bool VulkanContext::CreateSurface(GLFWwindow* window, int width, int height) {
  VkSurfaceKHR surface;
  VkResult err = glfwCreateWindowSurface(instance_, window, nullptr, &surface);
  if (err != VK_SUCCESS) {
    LOG(0) << "glfwCreateWindowSurface failed with error "
           << std::to_string(err);
    return false;
  }

  if (!queues_initialized_ && !InitializeQueues(surface))
    return false;

  window_.surface = surface;
  window_.width = width;
  window_.height = height;
  if (!UpdateSwapChain(&window_))
    return false;

  return true;
}

}  // namespace eng
