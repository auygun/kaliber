#ifndef RENDER_RESOURCE_H
#define RENDER_RESOURCE_H

#include <array>
#include <memory>
#include <typeindex>
#include <typeinfo>

namespace eng {

class Renderer;

class RenderResource {
 public:
  RenderResource(unsigned resource_id,
                 std::shared_ptr<void> impl_data,
                 Renderer* renderer);
  virtual ~RenderResource();

  virtual void Destroy() = 0;

  bool IsValid() const { return valid_; }

  std::shared_ptr<void> impl_data() { return impl_data_; }

 protected:
  unsigned resource_id_ = 0;
  std::shared_ptr<void> impl_data_;  // For use in render thread only.
  bool valid_ = false;

  Renderer* renderer_ = nullptr;

  RenderResource(const RenderResource&) = delete;
  RenderResource& operator=(const RenderResource&) = delete;
};

class RenderResourceFactoryBase {
 public:
  RenderResourceFactoryBase(std::type_index resource_type)
      : resource_type_(resource_type) {}
  virtual ~RenderResourceFactoryBase() = default;

  virtual std::shared_ptr<eng::RenderResource>
  Create(unsigned id, std::shared_ptr<void> impl_data, Renderer* renderer) = 0;

  template <typename T>
  bool IsTypeOf() const {
    return resource_type_ == std::type_index(typeid(T));
  }

 private:
  std::type_index resource_type_;
};

template <typename T>
class RenderResourceFactory : public RenderResourceFactoryBase {
 public:
  RenderResourceFactory()
      : RenderResourceFactoryBase(std::type_index(typeid(T))) {}
  ~RenderResourceFactory() override = default;

  std::shared_ptr<eng::RenderResource> Create(unsigned id,
                                              std::shared_ptr<void> impl_data,
                                              Renderer* renderer) override {
    return std::make_shared<T>(id, impl_data, renderer);
  }
};

}  // namespace eng

#endif  // RENDER_RESOURCE_H
