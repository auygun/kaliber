#ifndef SOLID_QUAD_H
#define SOLID_QUAD_H

#include "animatable.h"

namespace eng {

class SolidQuad : public Animatable {
 public:
  SolidQuad() = default;
  ~SolidQuad() override = default;

  // Animatable interface.
  void SetFrame(size_t frame) override {}
  size_t GetFrame() const override { return 0; }
  size_t GetNumFrames() const override { return 0; }
  void SetColor(const base::Vector4& color) override { color_ = color; }
  base::Vector4 GetColor() const override { return color_; }

  // Drawable interface.
  void Draw(float frame_frac) override;

 private:
  base::Vector4 color_ = {1, 1, 1, 1};
};

}  // namespace eng

#endif  // SOLID_QUAD_H
