#include "engine/renderer/vulkan/vulkan_context.h"

#include "base/log.h"
#include "third_party/glfw/glfw/include/GLFW/glfw3.h"

namespace eng {

void VulkanContext::GetRequiredInstanceExtensions(const char**& extensions,
                                                  uint32_t& count) const {
  // GLFW reports VK_KHR_surface plus the surface extension matching the
  // display backend it selected at runtime (X11 or Wayland).
  extensions = glfwGetRequiredInstanceExtensions(&count);
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
