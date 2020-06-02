#ifndef RENDER_RESOURCE_H
#define RENDER_RESOURCE_H

#include <memory>

namespace eng {

class RenderResource {
 public:
  RenderResource(unsigned resource_id);
  virtual ~RenderResource();

  virtual void Destroy() = 0;

  bool IsValid() const { return valid_; }

  void SetImplData(std::shared_ptr<void> impl_data) {
    impl_data_ = impl_data;
  }
  std::shared_ptr<void> GetImplData() { return impl_data_; }

 protected:
  unsigned resource_id_ = 0;
  std::shared_ptr<void> impl_data_;  // For use in render thread only.
  bool valid_ = false;

  RenderResource(const RenderResource&) = delete;
  RenderResource& operator=(const RenderResource&) = delete;
};

class RenderResourceFactoryBase {
 public:
  virtual ~RenderResourceFactoryBase() = default;

  virtual std::shared_ptr<eng::RenderResource> Create(unsigned id) = 0;
};

template <typename T>
class RenderResourceFactory : public RenderResourceFactoryBase {
 public:
  ~RenderResourceFactory() override = default;

  std::shared_ptr<eng::RenderResource> Create(unsigned id) override {
    return std::make_shared<T>(id);
  }
};

}  // namespace eng

#endif  // RENDER_RESOURCE_H
