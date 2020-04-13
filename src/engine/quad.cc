#include "quad.h"
#include "../base/log.h"

namespace engine {

bool Quad::Create() {
  // Create the shader we can reuse for all tiles.
  const char *vertexDescription = "p2f;t2f";
  if (!passThroughShader.Create("shaders/pass_through", vertexDescription)) {
    LOG("Could not create pass through shader.");
    return false;
  }

  // Create the quad geometry we can reuse for all tiles.
  // This creates a normalized unit sized quad.
  const float vertices[] =
  {
    -0.5f, -0.5f, 0.0f, 1.0f,
     0.5f, -0.5f, 1.0f, 1.0f,
    -0.5f,  0.5f, 0.0f, 0.0f,
     0.5f,  0.5f, 1.0f, 0.0f
  };
  quad.Create(GL_TRIANGLE_STRIP, vertexDescription, 4, vertices);

  return true;
}

void Quad::Activate() {
  passThroughShader.Activate();
  passThroughShader.SetUniform("tileImage", 0);
}

void Quad::Draw(const Vector2 &offset, const Vector2 &scale, const Vector3 &color) {
  passThroughShader.SetUniform("offset", offset);
  passThroughShader.SetUniform("scale", scale);
  passThroughShader.SetUniform("tileColor", color);

  quad.Draw();
}

} // namespace engine
