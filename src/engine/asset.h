#ifndef ASSET_H
#define ASSET_H

#include <string>

namespace eng {

class Asset {
 public:
  Asset() = default;
  virtual ~Asset() = default;

  virtual bool Load(const std::string& file_name) = 0;

  void SetName(const std::string& name) { name_ = name; }
  const std::string& GetName() const { return name_; }

  void SetImmutable() { immutable_ = true; }
  bool IsImmutable() const { return immutable_; }

 protected:
  std::string name_;
  bool immutable_ = false;
};

class AssetFactoryBase {
 public:
  AssetFactoryBase(const std::string& name) : name_(name) {}
  virtual ~AssetFactoryBase() = default;

  virtual std::shared_ptr<eng::Asset> Create() = 0;

  const std::string& name() { return name_; };

 private:
  std::string name_;
};

template <typename T>
class AssetFactory : public AssetFactoryBase {
 public:
  ~AssetFactory() override = default;

  AssetFactory(const std::string& name) : AssetFactoryBase(name) {}

  std::shared_ptr<eng::Asset> Create() override {
    return std::make_shared<T>();
  }
};

}  // namespace eng

#endif  // ASSET_H
