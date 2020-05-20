#ifndef COLLUSION_TEST_H
#define COLLUSION_TEST_H

#include "vecmath.h"

namespace base {

// Ray-AABB intersection test.
// center, size: Center and size of the box.
// origin, dir: Origin and direction of the ray.
bool Intersection(Vector2 center, Vector2 size, Vector2 origin, Vector2 dir);

}  // namespace base

#endif  // COLLUSION_TEST_H
