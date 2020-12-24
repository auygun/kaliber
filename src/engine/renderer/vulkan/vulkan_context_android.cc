#include "vulkan_context.h"

#include "../../../base/log.h"

namespace eng {

const char* VulkanContext::GetPlatformSurfaceExtension() const {
  return VK_KHR_ANDROID_SURFACE_EXTENSION_NAME;
}

bool VulkanContext::CreateWindow(ANativeWindow* window, int width, int height) {
  VkAndroidSurfaceCreateInfoKHR surface_info;
  surface_info.sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR;
  surface_info.pNext = nullptr;
  surface_info.flags = 0;
  surface_info.window = window;

  VkSurfaceKHR surface;
  VkResult err =
      vkCreateAndroidSurfaceKHR(instance_, &surface_info, nullptr, &surface);
  if (err != VK_SUCCESS) {
    LOG << "vkCreateAndroidSurfaceKHR failed with error "
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
