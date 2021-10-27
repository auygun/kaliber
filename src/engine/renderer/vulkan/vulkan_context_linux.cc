#include "engine/renderer/vulkan/vulkan_context.h"

#include "base/log.h"

namespace eng {

const char* VulkanContext::GetPlatformSurfaceExtension() const {
  return VK_KHR_XLIB_SURFACE_EXTENSION_NAME;
}

bool VulkanContext::CreateWindow(Display* display,
                                 ::Window window,
                                 int width,
                                 int height) {
  VkXlibSurfaceCreateInfoKHR surface_info;
  surface_info.sType = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR;
  surface_info.pNext = nullptr;
  surface_info.flags = 0;
  surface_info.dpy = display;
  surface_info.window = window;

  VkSurfaceKHR surface;
  VkResult err =
      vkCreateXlibSurfaceKHR(instance_, &surface_info, nullptr, &surface);
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
