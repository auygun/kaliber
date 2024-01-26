#ifndef ENGINE_RENDERER_RENDER_RESOURCE_H
#define ENGINE_RENDERER_RENDER_RESOURCE_H

#include <cstdint>

#include "base/log.h"

namespace eng {

class Renderer;

class RenderResource {
 public:
  RenderResource(const RenderResource&) = delete;
  RenderResource& operator=(const RenderResource&) = delete;

  bool IsValid() const { return resource_id_ != 0; }

  uint64_t resource_id() { return resource_id_; }

  void SetRenderer(Renderer* renderer) {
    renderer_ = renderer;
    resource_id_ = 0;
  }

 protected:
  uint64_t resource_id_;
  Renderer* renderer_;

  RenderResource() = default;
  RenderResource(Renderer* renderer) : resource_id_(0), renderer_(renderer){};
  ~RenderResource() { DCHECK(!IsValid()); }

  void Move(RenderResource& other) {
    resource_id_ = other.resource_id_;
    renderer_ = other.renderer_;
    other.SetRenderer(nullptr);
  }
};

}  // namespace eng

#endif  // ENGINE_RENDERER_RENDER_RESOURCE_H
