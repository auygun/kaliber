#ifndef IMAGE_QUAD_H
#define IMAGE_QUAD_H

#include "../base/vecmath.h"
#include "renderer/texture.h"

#include <string>
#include <vector>

namespace engine {

class ImageQuad {
 public:
  ImageQuad() = default;
  ~ImageQuad() = default;

  bool Create(const std::string& asset_name, const Vector2& offset);

  bool Print(const std::string& text, const Vector2& offset);
  bool Print(const std::vector<std::string> lines, int width,
             const Vector2& offset);

  void Draw(const Vector2& offset);

  void SetScale(const Vector2& scale) { scale_ = scale; }

  Vector2 GetOffset() { return offset_; }
  Vector2 GetScale() { return scale_; }

 private:
  Texture texture_;
  Vector2 offset_;
  Vector2 scale_;
};

}  // namespace engine

#endif  // IMAGE_QUAD_H
