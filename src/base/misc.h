#ifndef MISC_H
#define MISC_H

#include "vecmath.h"

#define CRASH *((int*)nullptr) = 0;

namespace base {

// ToDo: x86 has the bsr instruction.
inline int GetHighestBitPos(int value) {
  return (0xFFFF0000 & value ? value &= 0xFFFF0000, 1 : 0) * 0x10 +
         (0xFF00FF00 & value ? value &= 0xFF00FF00, 1 : 0) * 0x08 +
         (0xF0F0F0F0 & value ? value &= 0xF0F0F0F0, 1 : 0) * 0x04 +
         (0xCCCCCCCC & value ? value &= 0xCCCCCCCC, 1 : 0) * 0x02 +
         (0xAAAAAAAA & value ? 1 : 0) * 0x01;
}

// Get the highest set bit in an integer number
inline int GetHighestBit(int value) {
  return 0x1 << GetHighestBitPos(value);
}

// Check if the given integer is a power of two, ie if only one bit is set.
inline bool IsPow2(int value) {
  return GetHighestBit(value) == value;
  // return ((value & (value - 1)) == 0);
}

inline int RoundUpToPow2(int val) {
  int i = 1 << GetHighestBitPos(val);
  return val == i ? val : i << 1;
}

// Round a float to int.
inline int Round(float f) {
  return int(f + 0.5f);
}

// Linearly interpolate between a and b, by fraction t.
template <class T>
inline T Lerp(const T& a, const T& b, float t) {
  return a + (b - a) * t;
}

inline float BlendColorChannel(float c1, float c2, float t) {
  return sqrt((1 - t) * c1 * c1 + t * c2 * c2);
}

inline float BlendAlphaChannel(float a1, float a2, float t) {
  return (1 - t) * a1 + t * a2;
}

// Blend colors between a and b, by fraction t.
inline Vector4 Blend(const Vector4& c1, const Vector4& c2, float t) {
  return Vector4(
      BlendColorChannel(c1.x, c2.x, t), BlendColorChannel(c1.y, c2.y, t),
      BlendColorChannel(c1.z, c2.z, t), BlendAlphaChannel(c1.w, c2.w, t));
}

}  // namespace base

#endif  // MISC_H
