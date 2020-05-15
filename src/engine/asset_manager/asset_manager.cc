#include "asset_manager.h"
#include "image.h"
#include "font.h"

namespace eng {

std::shared_ptr<const Image> AssetManager::GetImage(const std::string& name) {
  auto it = images_.find(name);
  if (it != images_.end())
    return it->second;

  auto image = std::make_shared<Image>();
  if (!image->Load(name.c_str()))
    return nullptr;
  image->SetImmutable();

  images_[name] = image;
  return image;
}

std::shared_ptr<Font> AssetManager::GetFont(const std::string& name) {
  auto it = fonts_.find(name);
  if (it != fonts_.end())
    return it->second;

  auto font = std::make_shared<Font>();
  if (!font->Create(name.c_str()))
    return nullptr;

  fonts_[name] = font;
  return font;
}

}  // namespace eng
