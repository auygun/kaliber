#ifndef INTERPOLATION_H
#define INTERPOLATION_H

#include "vecmath.h"

namespace base {

// Linearly interpolate between a and b, by fraction t.
template <class T>
inline T Lerp(const T& a, const T& b, float t) {
  return a + (b - a) * t;
}

inline float SmoothStep(float t) {
  return t * t * (3 - 2 * t);
}

inline float SmootherStep(float t) {
  return t * t * t * (t * (t * 6 - 15) + 10);
}

// Interpolating spline defined by four control points with the curve drawn only
// from 0 to 1 which are p1 and p2 respectively.
inline float CatmullRom(float t, float p0, float p3) {
  return 0.5f * ((-p0 + 1) * t +
                 (2 * p0 + 4 * 1 - p3) * t * t +
                 (-p0 - 3 * 1 + p3) * t * t * t);
}

inline float Acceleration(float t) {
  return t * t;
}

inline float Deceleration(float t) {
  return 1 - (1 - t) * (1 - t);
}

}  // namespace base

#endif  // INTERPOLATION_H
