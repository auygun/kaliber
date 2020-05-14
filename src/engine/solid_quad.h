#ifndef SOLID_QUAD_H
#define SOLID_QUAD_H

#include "../base/vecmath.h"
#include "shape.h"

#include <string>
#include <vector>
#include <memory>
#include <array>

namespace eng {

class Image;

class SolidQuad : public Shape {
 public:
  SolidQuad() = default;
  ~SolidQuad() override = default;

  SolidQuad(const SolidQuad&) = delete;
  SolidQuad& operator=(const SolidQuad&) = delete;

  // Shape interface.
  void SetFrame(size_t frame) override {}
  size_t GetFrame() override { return 0; }
  size_t GetNumFrames() override { return 0; }

  // Drawable interface.
  void Draw() override;
  void ContextLost() override {}
};

}  // namespace eng

#endif  // SOLID_QUAD_H
