#ifndef INTERPOLATION_H
#define INTERPOLATION_H

#include "vecmath.h"

namespace base {

// Linearly interpolate between a and b, by fraction t.
template <class T>
inline T Lerp(const T& a, const T& b, float t) {
  return a + (b - a) * t;
}

inline float BlendColorChannel(float c1, float c2, float t) {
  return sqrt(Lerp(c1 * c1, c2 * c2, t));
}

// Blend colors between a and b, by fraction t.
inline Vector4 Blend(const Vector4& c1, const Vector4& c2, float t) {
  return Vector4(
      BlendColorChannel(c1.x, c2.x, t), BlendColorChannel(c1.y, c2.y, t),
      BlendColorChannel(c1.z, c2.z, t), Lerp(c1.w, c2.w, t));
}

inline float SmoothStep(float t) {
  return t * t * (3 - 2 * t);
}

inline float SmootherStep(float t) {
  return t * t * t * (t * (t * 6 - 15) + 10);
}

// Interpolating spline defined by four control points with the curve drawn only
// from 0 to 1 (p1 to p2).
inline float CatmullRom(float t, float p0, float p3) {
  return 0.5f * ((-p0 + 1) * t +
                 (2 * p0 + 4 * 1 - p3) * t * t +
                 (-p0 - 3 * 1 + p3) * t * t * t);
}

inline float Acceleration(float t) {
  return t * t;
}

inline float Decelleration(float t) {
  return 1 - (1 - t) * (1 - t);
}

}  // namespace base

#endif  // INTERPOLATION_H
