#ifndef ENGINE_RENDERER_TEXTURE_H
#define ENGINE_RENDERER_TEXTURE_H

#include <memory>
#include <string>

#include "engine/renderer/render_resource.h"
#include "engine/renderer/renderer_types.h"

namespace eng {

class Image;
class Renderer;

class Texture : public RenderResource {
 public:
  Texture();
  explicit Texture(Renderer* renderer);
  ~Texture();

  Texture(Texture&& other);
  Texture& operator=(Texture&& other);

  void Update(std::unique_ptr<Image> image);
  void Update(int width,
              int height,
              ImageFormat format,
              size_t data_size,
              uint8_t* image_data);

  void Destroy();

  void Activate(uint64_t texture_unit);

  int GetWidth() const { return width_; }
  int GetHeight() const { return height_; }

 private:
  int width_ = 0;
  int height_ = 0;
};

}  // namespace eng

#endif  // ENGINE_RENDERER_TEXTURE_H
