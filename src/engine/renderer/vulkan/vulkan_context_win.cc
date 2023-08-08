#include "engine/renderer/vulkan/vulkan_context.h"

#include "base/log.h"

namespace eng {

const char* VulkanContext::GetPlatformSurfaceExtension() const {
  return VK_KHR_WIN32_SURFACE_EXTENSION_NAME;
}

bool VulkanContext::CreateSurface(HINSTANCE hInstance,
                                  HWND hWnd,
                                  int width,
                                  int height) {
  VkWin32SurfaceCreateInfoKHR create_info;
  create_info.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
  create_info.pNext = nullptr;
  create_info.flags = 0;
  create_info.hinstance = hInstance;
  create_info.hwnd = hWnd;

  VkSurfaceKHR surface;
  VkResult err =
      vkCreateWin32SurfaceKHR(instance_, &create_info, nullptr, &surface);
  if (err != VK_SUCCESS) {
    LOG(0) << "vkCreateWin32SurfaceKHR failed with error "
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
