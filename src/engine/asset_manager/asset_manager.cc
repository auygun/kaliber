#include "asset_manager.h"
#include "image.h"

namespace eng {

std::shared_ptr<const Image> AssetManager::GetImage(const std::string& name) {
  auto it = images_.find(name);
  if (it != images_.end())
    return it->second;

  auto image = std::make_shared<Image>();
  if (!image->Load(name.c_str()))
    return nullptr;

  images_[name] = image;
  return image;
}

}  // namespace eng
