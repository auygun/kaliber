#include "engine/renderer/vulkan/renderer_vulkan.h"

#include "base/log.h"
#include "engine/platform/platform.h"

namespace eng {

bool RendererVulkan::Initialize(Platform* platform) {
  LOG(0) << "Initializing renderer.";

  RECT rect;
  GetClientRect(platform->GetWindow(), &rect);

  if (!context_.Initialize()) {
    LOG(0) << "Failed to initialize Vulkan context.";
    return false;
  }
  if (!context_.CreateSurface(platform->GetInstance(), platform->GetWindow(),
                              rect.right, rect.bottom)) {
    LOG(0) << "Vulkan context failed to create window.";
    return false;
  }

  return InitializeInternal();
}

}  // namespace eng
