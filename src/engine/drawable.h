#ifndef DRAWABLE_H
#define DRAWABLE_H

#include "../base/vecmath.h"

namespace eng {

class Drawable {
 public:
  Drawable() = default;
  virtual ~Drawable();

  Drawable(const Drawable&) = delete;
  Drawable& operator=(const Drawable&) = delete;

  virtual void Draw() = 0;

  void SetVisible(bool visible) { visible_ = visible; }
  bool IsVisible() const { return visible_; }

 private:
  bool visible_ = false;
};

}  // namespace eng

#endif  // DRAWABLE_H
