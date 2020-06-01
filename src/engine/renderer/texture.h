#ifndef TEXTURE_H
#define TEXTURE_H

#include <memory>
#include <string>

namespace eng {

class Image;

class Texture {
 public:
  Texture() = default;
  ~Texture();

  // Uploads image.
  void Update(std::shared_ptr<const Image> image);

  void Destroy();

  void Activate();

  void Invalidate() { resource_id_ = 0; }
  bool IsValid() const { return resource_id_ > 0; }

  int GetWidth() const { return width_; }
  int GetHeight() const { return height_; }

 private:
  int resource_id_ = 0;
  static int last_id;

  int width_ = 0;
  int height_ = 0;

  Texture(const Texture&) = delete;
  Texture& operator=(const Texture&) = delete;
};

}  // namespace eng

#endif  // TEXTURE_H
