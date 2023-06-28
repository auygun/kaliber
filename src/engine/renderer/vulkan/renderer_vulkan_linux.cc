#include "engine/renderer/vulkan/renderer_vulkan.h"

#include "base/log.h"
#include "engine/platform/platform.h"

namespace eng {

bool RendererVulkan::Initialize(Platform* platform) {
  LOG(0) << "Initializing renderer.";

  XWindowAttributes xwa;
  XGetWindowAttributes(platform->GetDisplay(), platform->GetWindow(), &xwa);
  screen_width_ = xwa.width;
  screen_height_ = xwa.height;

  if (!context_.Initialize()) {
    LOG(0) << "Failed to initialize Vulkan context.";
    return false;
  }
  if (!context_.CreateWindow(platform->GetDisplay(), platform->GetWindow(),
                             screen_width_, screen_height_)) {
    LOG(0) << "Vulkan context failed to create window.";
    return false;
  }

  return InitializeInternal();
}

}  // namespace eng
