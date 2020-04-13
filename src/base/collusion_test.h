#ifndef COLLUSION_TEST_H
#define COLLUSION_TEST_H

#include "vecmath.h"

namespace base {

// AABB vs point.
bool Intersection(const Vector2& center,
                  const Vector2& size,
                  const Vector2& point);

// Ray-AABB intersection test.
// center, size: Center and size of the box.
// origin, dir: Origin and direction of the ray.
bool Intersection(const Vector2& center,
                  const Vector2& size,
                  const Vector2& origin,
                  const Vector2& dir);

}  // namespace base

#endif  // COLLUSION_TEST_H
