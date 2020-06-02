#ifndef TEXTURE_H
#define TEXTURE_H

#include <memory>
#include <string>

#include "render_resource.h"

namespace eng {

class Image;
class Renderer;

class Texture : public RenderResource {
 public:
  Texture(unsigned resource_id,
          std::shared_ptr<void> impl_data,
          Renderer* renderer);
  ~Texture() override;

  // Uploads image.
  void Update(std::shared_ptr<const Image> image);

  void Destroy() override;

  void Activate();

  int GetWidth() const { return width_; }
  int GetHeight() const { return height_; }

 private:
  int width_ = 0;
  int height_ = 0;
};

}  // namespace eng

#endif  // TEXTURE_H
