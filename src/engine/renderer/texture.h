#ifndef TEXTURE_H
#define TEXTURE_H

#include <memory>

class Image;

namespace engine {

class Texture {
public:
  Texture() = default;
  ~Texture();
  
  bool Create(std::unique_ptr<Image> image);
  void Destroy();

  void Activate();

private:
  int id = 0;
  static int last_id;
};

} // namespace engine

#endif // TEXTURE_H
