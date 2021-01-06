#include "renderer_vulkan.h"

#include "../../../base/log.h"

namespace eng {

bool RendererVulkan::Initialize(Display* display, Window window) {
  LOG << "Initializing renderer.";

  XWindowAttributes xwa;
  XGetWindowAttributes(display, window, &xwa);
  screen_width_ = xwa.width;
  screen_height_ = xwa.height;

  if (!context_.Initialize()) {
    LOG << "Failed to initialize Vulkan context.";
    return false;
  }
  if (!context_.CreateWindow(display, window, screen_width_, screen_height_)) {
    LOG << "Vulkan context failed to create window.";
    return false;
  }

  return InitializeInternal();
}

XVisualInfo* RendererVulkan::GetXVisualInfo(Display* display) {
  long visual_mask = VisualScreenMask;
  int num_visuals;
  XVisualInfo visual_info_template = {};
  visual_info_template.screen = DefaultScreen(display);
  return XGetVisualInfo(display, visual_mask, &visual_info_template,
                        &num_visuals);
}

}  // namespace eng
