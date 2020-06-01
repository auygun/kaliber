#ifndef RENDER_RESOURCE_H
#define RENDER_RESOURCE_H

#include <memory>

namespace eng {

class Asset;

class RenderResource {
 public:
  RenderResource() = default;
  virtual ~RenderResource() = default;

  int GetResourceId() const { return resource_id_; }

  void Invalidate() { resource_id_ = 0; }
  bool IsValid() const { return resource_id_ > 0; }

 protected:
  int resource_id_ = 0;

  RenderResource(const RenderResource&) = delete;
  RenderResource& operator=(const RenderResource&) = delete;
};

class RenderResourceFactoryBase {
 public:
  RenderResourceFactoryBase() = default;
  virtual ~RenderResourceFactoryBase() = default;

  virtual std::shared_ptr<eng::RenderResource> Create() = 0;
};

template <typename T>
class RenderResourceFactory : public RenderResourceFactoryBase {
 public:
  RenderResourceFactory() = default;
  ~RenderResourceFactory() override = default;

  std::shared_ptr<eng::RenderResource> Create() override {
    return std::make_shared<T>();
  }
};

}  // namespace eng

#endif  // RENDER_RESOURCE_H
