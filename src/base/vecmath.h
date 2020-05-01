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

  Vector2 operator-() { return Vector2(x * -1.0f, y * -1.0f); }

  Vector2 operator+=(const Vector2& v) { x += v.x; y += v.y; return *this; }

  Vector2 operator-=(const Vector2& v) { x -= v.x; y -= v.y; return *this; }

  Vector2 operator*=(const Vector2& v) { x *= v.x; y *= v.y; return *this; }

  Vector2 operator*=(float s) { x *= s; y *= s; return *this; }

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

class Vector3 {
 public:
  float x, y, z;

  Vector3() {}
  Vector3(float _x, float _y, float _z) : x(_x), y(_y), z(_z) {}

  const float* GetData() const { return &x; }
};

class Vector4 {
 public:
  float x, y, z, w;

  Vector4() {}
  Vector4(float _x, float _y, float _z, float _w) : x(_x), y(_y), z(_z), w(_w) {}

  const float* GetData() const { return &x; }
};

class Matrix4x4 {
 public:
  Vector4 col[4];

  Matrix4x4() {}
  Matrix4x4(float s)
    : col{Vector4(s, 0, 0, 0),
          Vector4(0, s, 0, 0),
          Vector4(0, 0, s, 0),
          Vector4(0, 0, 0, s)} {}

  const float* GetData() const { return &col[0].x; }
};

inline Matrix4x4 Ortho(float left, float right, float bottom, float top) {
  Matrix4x4 m(1);
  m.col[0].x = 2.0f / (right - left);
  m.col[1].y = 2.0f / (top - bottom);
  m.col[2].z = - 1.0f;
  m.col[3].x = - (right + left) / (right - left);
  m.col[3].y = - (top + bottom) / (top - bottom);
  return m;
}

#endif  // VEC_MATH_H
