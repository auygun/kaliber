#ifndef ASSET_MANAGER_H
#define ASSET_MANAGER_H

#include <cstdlib>
#include <string>
#include <memory>
#include <unordered_map>

class Image;

namespace engine {

// Manages shared assets. Assets returned from this class are immutable so they
// can be shared/accessed between multiple threads without locaking.
class AssetManager {
 public:
  AssetManager() = default;
  ~AssetManager() = default;

  std::shared_ptr<const Image> GetImage(const std::string& name);

 private:
  std::unordered_map<std::string, std::shared_ptr<const Image>> images_;
};

}  // namespace engine

#endif  // ASSET_MANAGER_H
