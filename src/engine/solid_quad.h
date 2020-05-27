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
  size_t GetFrame() override { return 0; }
  size_t GetNumFrames() override { return 0; }
  void SetColor(const base::Vector4& color) override { color_ = color; }
  base::Vector4 GetColor() const override { return color_; }

  void Draw();

 private:
  base::Vector4 color_ = {1, 1, 1, 1};
};

}  // namespace eng

#endif  // SOLID_QUAD_H
