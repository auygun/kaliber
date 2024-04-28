#include "engine/renderer/vulkan/vulkan_context.h"

#include "base/log.h"
#include "third_party/vulkan/vk_enum_string_helper.h"

namespace eng {

const char* VulkanContext::GetPlatformSurfaceExtension() const {
  std::uint32_t num_instance_extensions = 0;
  auto required_instance_extensions =
      glfwGetRequiredInstanceExtensions(&num_instance_extensions);
  LOG(0) << "glfwGetRequiredInstanceExtensions: " << num_instance_extensions;
  for (int i = 0; i < num_instance_extensions; ++i)
    LOG(0) << "ext: " << required_instance_extensions[i];
  return required_instance_extensions[1];
}

bool VulkanContext::CreateSurface(GLFWwindow* window, int width, int height) {
  VkSurfaceKHR surface;
  VkResult err = glfwCreateWindowSurface(instance_, window, nullptr, &surface);
  if (err != VK_SUCCESS) {
    LOG(0) << "glfwCreateWindowSurface failed with error "
           << string_VkResult(err);
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
