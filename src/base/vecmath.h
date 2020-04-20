#ifndef VEC_MATH_H
#define VEC_MATH_H

class Vector2 {
 public:
  float x, y;

  Vector2() {}
  Vector2(float _x, float _y) : x(_x), y(_y) {}

  Vector2 operator+(const Vector2& v) { return Vector2(x + v.x, y + v.y); }

  const float* GetData() const { return &x; }
};

class Vector3 {
 public:
  float x, y, z;

  Vector3() {}
  Vector3(float _x, float _y, float _z) : x(_x), y(_y), z(_z) {}

  const float* GetData() const { return &x; }
};

#endif  // VEC_MATH_H
