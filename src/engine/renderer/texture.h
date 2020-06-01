#ifndef TEXTURE_H
#define TEXTURE_H

#include <memory>
#include <string>

#include "render_resource.h"

namespace eng {

class Image;

class Texture : public RenderResource {
 public:
  Texture() = default;
  ~Texture() override;

  // Uploads image.
  void Update(std::shared_ptr<const Image> image);

  void Destroy();

  void Activate();

  int GetWidth() const { return width_; }
  int GetHeight() const { return height_; }

 private:
  static int last_id;

  int width_ = 0;
  int height_ = 0;

  Texture(const Texture&) = delete;
  Texture& operator=(const Texture&) = delete;
};

}  // namespace eng

#endif  // TEXTURE_H
