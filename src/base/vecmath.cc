#include "vecmath.h"

using namespace std::string_literals;

namespace base {

std::string Vector2::ToString() {
  return "("s + std::to_string(x) + ", "s + std::to_string(y) + ")"s;
}

Matrix4x4 Ortho(float left, float right, float bottom, float top) {
  Matrix4x4 m(1);
  m.c[0].x = 2.0f / (right - left);
  m.c[1].y = 2.0f / (top - bottom);
  m.c[2].z = -1.0f;
  m.c[3].x = -(right + left) / (right - left);
  m.c[3].y = -(top + bottom) / (top - bottom);
  return m;
}

}  // namespace base
