#ifndef VEC_MATH_H
#define VEC_MATH_H

#include <math.h>

class Vector2 {
 public:
  float x, y;

  Vector2() {}
  Vector2(float _x, float _y) : x(_x), y(_y) {}

  float Magnitude() { return sqrt(x * x + y * y); }

  Vector2 Normalize() { float m = Magnitude(); x /= m; y /= m; return *this; }

  float DotProduct(const Vector2& v) { return x * v.x + y * v.y; }

  Vector2 operator+=(const Vector2& v) { x += v.x; y += v.y; return *this; }

  Vector2 operator*=(float s) { x *= s; y *= s; return *this; }

  const float* GetData() const { return &x; }
};

class Vector3 {
 public:
  float x, y, z;

  Vector3() {}
  Vector3(float _x, float _y, float _z) : x(_x), y(_y), z(_z) {}

  const float* GetData() const { return &x; }
};

inline Vector2 operator+(const Vector2& v1, const Vector2& v2) {
  return Vector2(v1.x + v2.x, v1.y + v2.y);
}

inline Vector2 operator-(const Vector2& v1, const Vector2& v2) {
  return Vector2(v1.x - v2.x, v1.y - v2.y);
}

inline Vector2 operator*(const Vector2& v1, const Vector2& v2) {
  return Vector2(v1.x * v2.x, v1.y * v2.y);
}

inline Vector2 operator/(const Vector2& v1, const Vector2& v2) {
  return Vector2(v1.x / v2.x, v1.y / v2.y);
}

inline Vector2 operator*(const Vector2& v, float s)
{
  return Vector2(v.x * s, v.y * s);
}

inline Vector2 operator/(const Vector2& v, float s)
{
  return Vector2(v.x / s, v.y / s);
}

#endif  // VEC_MATH_H
