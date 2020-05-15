#ifndef ASSET_MANAGER_H
#define ASSET_MANAGER_H

#include <cstdlib>
#include <string>
#include <memory>
#include <unordered_map>

namespace eng {

class Image;
class Font;

// Manages shared assets.
class AssetManager {
 public:
  AssetManager() = default;
  ~AssetManager() = default;

  // Returns immutable image that can be accessed between multiple threads
  // without locking.
  std::shared_ptr<const Image> GetImage(const std::string& name);

  std::shared_ptr<Font> GetFont(const std::string& name);

 private:
  std::unordered_map<std::string, std::shared_ptr<const Image>> images_;
  std::unordered_map<std::string, std::shared_ptr<Font>> fonts_;
};

}  // namespace eng

#endif  // ASSET_MANAGER_H
