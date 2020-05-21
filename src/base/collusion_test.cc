#include "collusion_test.h"

#include <algorithm>
#include <limits>

namespace base {

bool Intersection(Vector2 center, Vector2 size, Vector2 origin, Vector2 dir) {
  Vector2 min = center - size / 2;
  Vector2 max = center + size / 2;

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
