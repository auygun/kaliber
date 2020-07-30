#ifndef DRAWABLE_H
#define DRAWABLE_H

#include "../base/vecmath.h"

namespace eng {

class Drawable {
 public:
  Drawable();
  virtual ~Drawable();

  Drawable(const Drawable&) = delete;
  Drawable& operator=(const Drawable&) = delete;

  virtual void Draw(float frame_frac) = 0;

  void SetZOrder(int z) { z_order_ = z; }
  void SetVisible(bool visible) { visible_ = visible; }

  int GetZOrder() const { return z_order_; }
  bool IsVisible() const { return visible_; }

 private:
  bool visible_ = false;
  int z_order_ = 0;
};

}  // namespace eng

#endif  // DRAWABLE_H
