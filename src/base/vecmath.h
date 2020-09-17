#ifndef VEC_MATH_H
#define VEC_MATH_H

#include <algorithm>
#include <cmath>
#include <string>

namespace base {

struct Vector2 {
  float x, y;

  Vector2() {}
  Vector2(float _x, float _y) : x(_x), y(_y) {}

  float Magnitude() { return sqrt(x * x + y * y); }

  Vector2 Normalize() {
    float m = Magnitude();
    x /= m;
    y /= m;
    return *this;
  }

  float DotProduct(const Vector2& v) { return x * v.x + y * v.y; }

  float CrossProduct(const Vector2& v) { return x * v.y - y * v.x; }

  Vector2 operator-() { return Vector2(x * -1.0f, y * -1.0f); }

  Vector2 operator+=(const Vector2& v) {
    x += v.x;
    y += v.y;
    return *this;
  }

  Vector2 operator-=(const Vector2& v) {
    x -= v.x;
    y -= v.y;
    return *this;
  }

  Vector2 operator*=(const Vector2& v) {
    x *= v.x;
    y *= v.y;
    return *this;
  }

  Vector2 operator*=(float s) {
    x *= s;
    y *= s;
    return *this;
  }

  Vector2 operator/=(const Vector2& v) {
    x /= v.x;
    y /= v.y;
    return *this;
  }

  Vector2 operator/=(float s) {
    x /= s;
    y /= s;
    return *this;
  }

  const float* GetData() const { return &x; }

  std::string ToString();
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
  Vector4(float _x, float _y, float _z, float _w)
      : x(_x), y(_y), z(_z), w(_w) {}

  Vector4 operator+=(const Vector4& v) {
    x += v.x;
    y += v.y;
    z += v.z;
    w += v.w;
    return *this;
  }

  const float* GetData() const { return &x; }
};

inline Vector4 operator*(const Vector4& v1, const Vector4& v2) {
  return Vector4(v1.x * v2.x, v1.y * v2.y, v1.z * v2.z, v1.w * v2.w);
}

inline Vector4 operator*(const Vector4& v, float s) {
  return Vector4(v.x * s, v.y * s, v.z * s, v.w * s);
}

inline Vector4 operator+(const Vector4& v1, const Vector4& v2) {
  return Vector4(v1.x + v2.x, v1.y + v2.y, v1.z + v2.z, v1.w + v2.w);
}

inline Vector4 operator-(const Vector4& v1, const Vector4& v2) {
  return Vector4(v1.x - v2.x, v1.y - v2.y, v1.z - v2.z, v1.w - v2.w);
}

struct Matrix4x4 {
  Vector4 c[4];

  Matrix4x4() {}
  Matrix4x4(float s)
      : c{Vector4(s, 0, 0, 0), Vector4(0, s, 0, 0), Vector4(0, 0, s, 0),
          Vector4(0, 0, 0, s)} {}

  const float* GetData() const { return &c[0].x; }
};

Matrix4x4 Ortho(float left, float right, float bottom, float top);

}  // namespace base

#endif  // VEC_MATH_H
