#include "engine/renderer/renderer.h"

#include "base/log.h"
#include "engine/renderer/opengl/renderer_opengl.h"
#include "engine/renderer/vulkan/renderer_vulkan.h"

namespace eng {

// static
std::unique_ptr<Renderer> Renderer::Create(RendererType type,
                                           base::Closure context_lost_cb) {
  std::unique_ptr<Renderer> renderer;
  if (type == RendererType::kVulkan) {
    renderer = std::make_unique<RendererVulkan>(std::move(context_lost_cb));
  } else if (type == RendererType::kOpenGL) {
    renderer = std::make_unique<RendererOpenGL>(std::move(context_lost_cb));
  } else {
    NOTREACHED();
  }
  return renderer;
}

}  // namespace eng
