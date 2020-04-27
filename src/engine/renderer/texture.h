#ifndef TEXTURE_H
#define TEXTURE_H

#include <memory>

namespace engine {

class Image;

class Texture {
public:
  Texture() = default;
  ~Texture();

  bool Create(std::shared_ptr<const Image> image);
  void Destroy();

  void Activate();

  bool IsValid() { return id > 0; }

private:
  int id = 0; // TODO: ResourceId
  static int last_id;
};

} // namespace engine

#endif // TEXTURE_H
