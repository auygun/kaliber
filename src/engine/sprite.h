#ifndef SPRITE_H
#define SPRITE_H

#include "../base/image.h"
#include "../base/vecmath.h"
#include "renderer/itexture.h"

#include <string>

namespace engine {

class Sprite {
 public:
  Sprite() = default;
  ~Sprite() = default;

  bool Create(const std::string& asset_name,
              const Vector2& offset,
              const Vector2& scale);

  void Draw(const Vector2& offset);

  Vector2 GetOffset() { return offset_; }
  Vector2 GetScale() { return scale_; }

 private:
  ITexture texture_;
  Vector2 offset_;
  Vector2 scale_;
};

}  // namespace engine

#endif  // SPRITE_H
