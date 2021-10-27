#ifndef BASE_COLLUSION_TEST_H
#define BASE_COLLUSION_TEST_H

#include "base/vecmath.h"

namespace base {

// AABB vs point.
bool Intersection(const Vector2f& center,
                  const Vector2f& size,
                  const Vector2f& point);

// Ray-AABB intersection test.
// center, size: Center and size of the box.
// origin, dir: Origin and direction of the ray.
bool Intersection(const Vector2f& center,
                  const Vector2f& size,
                  const Vector2f& origin,
                  const Vector2f& dir);

}  // namespace base

#endif  // BASE_COLLUSION_TEST_H
