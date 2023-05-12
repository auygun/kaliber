#ifndef ENGINE_SOLID_QUAD_H
#define ENGINE_SOLID_QUAD_H

#include "engine/animatable.h"

namespace eng {

class SolidQuad final : public Animatable {
 public:
  SolidQuad() = default;
  ~SolidQuad() final = default;

  // Animatable interface.
  void SetFrame(size_t frame) final {}
  size_t GetFrame() const final { return 0; }
  size_t GetNumFrames() const final { return 0; }
  void SetColor(const base::Vector4f& color) final { color_ = color; }
  base::Vector4f GetColor() const final { return color_; }

  // Drawable interface.
  void Draw(float frame_frac) final;

 private:
  base::Vector4f color_ = {1, 1, 1, 1};
};

}  // namespace eng

#endif  // ENGINE_SOLID_QUAD_H
