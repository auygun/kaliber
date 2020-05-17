#ifndef ASSET_H
#define ASSET_H

#include <string>

namespace eng {

class Asset {
 public:
  Asset() = default;
  virtual ~Asset() = default;

  void SetName(const std::string& name) { name_ = name; }
  const std::string& GetName() const { return name_; }

  void SetImmutable() { immutable_ = true; }
  bool IsImmutable() const { return immutable_; }

 protected:
  std::string name_;
  bool immutable_ = false;
};

}  // namespace eng

#endif  // ASSET_H
