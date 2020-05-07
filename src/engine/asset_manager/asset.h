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

 private:
  std::string name_;
};

}  // namespace eng

#endif  // ASSET_H
