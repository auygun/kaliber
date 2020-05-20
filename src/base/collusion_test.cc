#include "collusion_test.h"

namespace base {

bool Intersection(Vector2 center,
                         Vector2 size,
                         Vector2 origin,
                         Vector2 dir) {
  Vector2 min = center - size / 2;
  Vector2 max = center + size / 2;

  float r_dir_inv_x = 1.0f / dir.x;
  float r_dir_inv_y = 1.0f / dir.y;

  float tx1 = (min.x - origin.x) * r_dir_inv_x;
  float tx2 = (max.x - origin.x) * r_dir_inv_x;

  float tmin = std::min(tx1, tx2);
  float tmax = std::max(tx1, tx2);

  float ty1 = (min.y - origin.y) * r_dir_inv_y;
  float ty2 = (max.y - origin.y) * r_dir_inv_y;

  tmin = std::max(tmin, std::min(ty1, ty2));
  tmax = std::min(tmax, std::max(ty1, ty2));

  return tmax >= tmin;
}

}  // namespace base
