#ifndef TEXTURE_H
#define TEXTURE_H

#include <memory>

namespace eng {

class Image;

class Texture {
public:
  Texture() = default;
  ~Texture();

  Texture(const Texture&) = delete;
  Texture& operator=(const Texture&) = delete;

  bool Create(std::shared_ptr<const Image> image);
  void Destroy();

  void Activate();

  bool IsValid() { return resource_id_ > 0; }

private:
  int resource_id_ = 0; // TODO: ResourceId
};

} // namespace eng

#endif // TEXTURE_H
