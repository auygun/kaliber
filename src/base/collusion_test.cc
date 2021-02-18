#include "collusion_test.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace base {

bool Intersection(const Vector2f& center,
                  const Vector2f& size,
                  const Vector2f& point) {
  float dx = point.x - center.x;
  float px = size.x / 2 - fabs(dx);
  if (px <= 0)
    return false;

  float dy = point.y - center.y;
  float py = size.y / 2 - fabs(dy);
  return py > 0;
}

bool Intersection(const Vector2f& center,
                  const Vector2f& size,
                  const Vector2f& origin,
                  const Vector2f& dir) {
  Vector2f min = center - size / 2;
  Vector2f max = center + size / 2;

  float tmin = std::numeric_limits<float>::min();
  float tmax = std::numeric_limits<float>::max();

  if (dir.x != 0.0) {
    float tx1 = (min.x - origin.x) / dir.x;
    float tx2 = (max.x - origin.x) / dir.x;

    tmin = std::max(tmin, std::min(tx1, tx2));
    tmax = std::min(tmax, std::max(tx1, tx2));
  }

  if (dir.y != 0.0) {
    float ty1 = (min.y - origin.y) / dir.y;
    float ty2 = (max.y - origin.y) / dir.y;

    tmin = std::max(tmin, std::min(ty1, ty2));
    tmax = std::min(tmax, std::max(ty1, ty2));
  }

  return tmax >= tmin;
}

}  // namespace base
