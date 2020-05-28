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

  // Use exsting resource. Returns false if no resource was found.
  bool Create(const std::string &name);

  // Update exsiting resource or create a new one if no resource was found.
  void Update(std::shared_ptr<const Image> image);

  void Destroy();

  void Activate();

  void Invalidate() { resource_id_ = 0; }
  bool IsValid() const { return resource_id_ > 0; }

 private:
  int resource_id_ = 0;

  Texture(const Texture&) = delete;
  Texture& operator=(const Texture&) = delete;
};

}  // namespace eng

#endif  // TEXTURE_H
