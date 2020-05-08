#ifndef SKY_QUAD_H
#define SKY_QUAD_H

#include "../base/vecmath.h"
#include "../engine/renderer/geometry.h"
#include "../engine/renderer/shader.h"
#include "../engine/drawable.h"

#include <string>
#include <vector>
#include <memory>
#include <array>

namespace eng {
class Image;
}  // namespace eng

class SkyQuad : public eng::Drawable {
 public:
  SkyQuad() = default;
  ~SkyQuad() override = default;

  SkyQuad(const SkyQuad&) = delete;
  SkyQuad& operator=(const SkyQuad&) = delete;

  bool Create();

  void ContextLost();

  // Drawable interface.
  void Draw() override;

 private:
  eng::Geometry quad_;
  eng::Shader shader_;
  Vector2 sky_offset_ = {0, 0};
  Vector3 nebula_color_ = {0, 0, 0};
  Vector2 scale_ = {1, 1};
};

#endif  // SKY_QUAD_H
