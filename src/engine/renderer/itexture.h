#ifndef ITEXTURE_H
#define ITEXTURE_H

#include <memory>

class Image;

namespace engine {

class ITexture {
public:
  ITexture() = default;
  ~ITexture();
  
  bool Create(std::unique_ptr<Image> image);
  void Destroy();

  void Activate();

private:
  int id = 0;
  static int last_id;
};

} // namespace engine

#endif // ITEXTURE_H
