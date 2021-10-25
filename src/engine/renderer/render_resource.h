#ifndef RENDER_RESOURCE_H
#define RENDER_RESOURCE_H

#include <cstdint>

namespace eng {

class Renderer;

class RenderResource {
 public:
  RenderResource(Renderer* renderer) : renderer_(renderer){};

  bool IsValid() const { return resource_id_ != 0; }

  uint64_t resource_id() { return resource_id_; }

 protected:
  uint64_t resource_id_ = 0;
  Renderer* renderer_ = nullptr;

  ~RenderResource() = default;

  RenderResource(const RenderResource&) = delete;
  RenderResource& operator=(const RenderResource&) = delete;
};

}  // namespace eng

#endif  // RENDER_RESOURCE_H
