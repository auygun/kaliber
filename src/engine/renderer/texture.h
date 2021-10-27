#ifndef ENGINE_RENDERER_TEXTURE_H
#define ENGINE_RENDERER_TEXTURE_H

#include <memory>
#include <string>

#include "engine/renderer/render_resource.h"

namespace eng {

class Image;
class Renderer;

class Texture : public RenderResource {
 public:
  Texture(Renderer* renderer);
  ~Texture();

  void Update(std::unique_ptr<Image> image);

  void Destroy();

  void Activate();

  int GetWidth() const { return width_; }
  int GetHeight() const { return height_; }

 private:
  int width_ = 0;
  int height_ = 0;
};

}  // namespace eng

#endif  // ENGINE_RENDERER_TEXTURE_H
