#ifndef QUAD_H
#define QUAD_H

#include "renderer/geometry.h"
#include "renderer/shader.h"
#include "vecmath.h"

namespace engine {

class Quad {
public:
  Quad() = default;
  ~Quad() = default;

  bool Create();

  void Activate();
  void Draw(const Vector2 &offset, const Vector2 &scale, const Vector3 &color);

private:
  Geometry quad;
  Shader passThroughShader;
};

} // namespace engine

#endif // QUAD_H
