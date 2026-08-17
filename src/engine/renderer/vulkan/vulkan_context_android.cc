#include "engine/renderer/vulkan/vulkan_context.h"

#include <iterator>

#include "base/log.h"

namespace eng {

void VulkanContext::GetRequiredInstanceExtensions(const char**& extensions,
                                                  uint32_t& count) const {
  static const char* kExtensions[] = {
      VK_KHR_SURFACE_EXTENSION_NAME,
      VK_KHR_ANDROID_SURFACE_EXTENSION_NAME,
  };
  extensions = kExtensions;
  count = static_cast<uint32_t>(std::size(kExtensions));
}

bool VulkanContext::CreateSurface(ANativeWindow* window,
                                  int width,
                                  int height) {
  VkAndroidSurfaceCreateInfoKHR surface_info{};
  surface_info.sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR;
  surface_info.window = window;

  VkSurfaceKHR surface;
  VkResult err =
      vkCreateAndroidSurfaceKHR(instance_, &surface_info, nullptr, &surface);
  if (err != VK_SUCCESS) {
    LOG(0) << "vkCreateAndroidSurfaceKHR failed with error "
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
