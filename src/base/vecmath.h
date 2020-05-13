#ifndef VEC_MATH_H
#define VEC_MATH_H

#include <algorithm>
#include <cmath>

struct Vector2 {
  float x, y;

  Vector2() {}
  Vector2(float _x, float _y) : x(_x), y(_y) {}

  float Magnitude() { return sqrt(x * x + y * y); }

  Vector2 Normalize() { float m = Magnitude(); x /= m; y /= m; return *this; }

  float DotProduct(const Vector2& v) { return x * v.x + y * v.y; }

  float CrossProduct(const Vector2& v) { return x * v.y - y * v.x; }

  Vector2 operator-() { return Vector2(x * -1.0f, y * -1.0f); }

  Vector2 operator+=(const Vector2& v) { x += v.x; y += v.y; return *this; }

  Vector2 operator-=(const Vector2& v) { x -= v.x; y -= v.y; return *this; }

  Vector2 operator*=(const Vector2& v) { x *= v.x; y *= v.y; return *this; }

  Vector2 operator*=(float s) { x *= s; y *= s; return *this; }

  Vector2 operator/=(const Vector2& v) { x /= v.x; y /= v.y; return *this; }

  Vector2 operator/=(float s) { x /= s; y /= s; return *this; }

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

inline Vector2 operator*(const Vector2& v, float s) {
  return Vector2(v.x * s, v.y * s);
}

inline Vector2 operator/(const Vector2& v, float s) {
  return Vector2(v.x / s, v.y / s);
}

inline bool operator==(const Vector2& v1, const Vector2& v2) {
  return v1.x == v2.x && v1.y == v2.y;
}

inline bool operator!=(const Vector2& v1, const Vector2& v2) {
  return v1.x != v2.x || v1.y != v2.y;
}

struct Vector3 {
  float x, y, z;

  Vector3() {}
  Vector3(float _x, float _y, float _z) : x(_x), y(_y), z(_z) {}

  const float* GetData() const { return &x; }
};

inline Vector3 operator+(const Vector3& v1, const Vector3& v2) {
  return Vector3(v1.x + v2.x, v1.y + v2.y, v1.z + v2.z);
}

struct Vector4 {
  float x, y, z, w;

  Vector4() {}
  Vector4(float _x, float _y, float _z, float _w) : x(_x), y(_y), z(_z), w(_w) {}

  Vector4 operator+=(const Vector4& v) { x += v.x; y += v.y; z += v.z; w += v.w; return *this; }

  const float* GetData() const { return &x; }
};

inline Vector4 operator*(const Vector4& v, float s) {
  return Vector4(v.x * s, v.y * s, v.z * s, v.w * s);
}

inline Vector4 operator-(const Vector4& v1, const Vector4& v2) {
  return Vector4(v1.x - v2.x, v1.y - v2.y, v1.z - v2.z, v1.w - v2.w);
}

struct Matrix4x4 {
  Vector4 c[4];

  Matrix4x4() {}
  Matrix4x4(float s)
    : c{Vector4(s, 0, 0, 0),
        Vector4(0, s, 0, 0),
        Vector4(0, 0, s, 0),
        Vector4(0, 0, 0, s)} {}

  const float* GetData() const { return &c[0].x; }
};

inline Matrix4x4 Ortho(float left, float right, float bottom, float top) {
  Matrix4x4 m(1);
  m.c[0].x = 2.0f / (right - left);
  m.c[1].y = 2.0f / (top - bottom);
  m.c[2].z = - 1.0f;
  m.c[3].x = - (right + left) / (right - left);
  m.c[3].y = - (top + bottom) / (top - bottom);
  return m;
}

// Ray-AABB intersection test.
// center, size: Center and size of the box.
// origin, dir: Origin and direction of the ray.
inline bool Intersection(Vector2 center, Vector2 size, Vector2 origin, Vector2 dir) {
  Vector2 min = center - size / 2;
  Vector2 max = center + size / 2;

  float r_dir_inv_x = 1.0f / dir.x;
  float r_dir_inv_y = 1.0f / dir.y;

  float tx1 = (min.x - origin.x)*r_dir_inv_x;
  float tx2 = (max.x - origin.x)*r_dir_inv_x;

  float tmin = std::min(tx1, tx2);
  float tmax = std::max(tx1, tx2);

  float ty1 = (min.y - origin.y)*r_dir_inv_y;
  float ty2 = (max.y - origin.y)*r_dir_inv_y;

  tmin = std::max(tmin, std::min(ty1, ty2));
  tmax = std::min(tmax, std::max(ty1, ty2));

  return tmax >= tmin;
}

#endif  // VEC_MATH_H
