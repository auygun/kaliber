attribute vec2 inPosition;
attribute vec2 inTexCoord0;

uniform vec2  scale;
uniform vec2  offset;

varying vec2  texCoord0;

void main() {
  // Simple 2d transform.
  vec2 position = inPosition;
  position *= scale;
  position += offset;

  texCoord0 = inTexCoord0;

  gl_Position = vec4(position, 0.0, 1.0);
}
