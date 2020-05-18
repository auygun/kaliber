#ifndef SKY_QUAD_H
#define SKY_QUAD_H

#include "../base/vecmath.h"
#include "../engine/renderer/shader.h"

#include <string>
#include <vector>
#include <memory>
#include <array>

namespace eng {
class Image;
}  // namespace eng

class SkyQuad {
 public:
  SkyQuad() = default;
  ~SkyQuad() = default;

  SkyQuad(const SkyQuad&) = delete;
  SkyQuad& operator=(const SkyQuad&) = delete;

  bool Create();

  void Draw();
  void ContextLost();

  void Translate(base::Vector2 offset) { sky_offset_ += offset; }

 private:
  eng::Shader shader_;
  base::Vector2 sky_offset_ = {0, 0};
  base::Vector3 nebula_color_ = {0, 0, 0};
  base::Vector2 scale_ = {1, 1};
};

#endif  // SKY_QUAD_H
