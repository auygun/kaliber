#include "engine/renderer/vulkan/renderer_vulkan.h"

#include "base/log.h"
#include "engine/platform/platform.h"

namespace eng {

bool RendererVulkan::Initialize(Platform* platform) {
  LOG(0) << "Initializing renderer.";

  XWindowAttributes xwa;
  XGetWindowAttributes(platform->GetDisplay(), platform->GetWindow(), &xwa);

  if (!context_.Initialize()) {
    LOG(0) << "Failed to initialize Vulkan context.";
    return false;
  }
  if (!context_.CreateWindow(platform->GetDisplay(), platform->GetWindow(),
                             xwa.width, xwa.height)) {
    LOG(0) << "Vulkan context failed to create window.";
    return false;
  }

  return InitializeInternal();
}

}  // namespace eng
