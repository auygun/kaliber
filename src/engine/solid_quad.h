#ifndef SOLID_QUAD_H
#define SOLID_QUAD_H

#include "../base/vecmath.h"
#include "animatable.h"

#include <string>
#include <vector>
#include <memory>
#include <array>

namespace eng {

class Image;

class SolidQuad : public Animatable {
 public:
  SolidQuad() = default;
  ~SolidQuad() override = default;

  SolidQuad(const SolidQuad&) = delete;
  SolidQuad& operator=(const SolidQuad&) = delete;

  // Shape interface.
  void SetFrame(size_t frame) override {}
  size_t GetFrame() override { return 0; }
  size_t GetNumFrames() override { return 0; }

  void Draw();

  void SetVisible(bool visible) { visible_ = visible; }
  bool IsVisible() const { return visible_; }

private:
  bool visible_ = false;
};

}  // namespace eng

#endif  // SOLID_QUAD_H
