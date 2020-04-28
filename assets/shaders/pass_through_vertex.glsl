attribute vec2 inPosition;
attribute vec2 inTexCoord0;

uniform vec2  scale;
uniform vec2  offset;
uniform vec2  rotation;

varying vec2  texCoord0;

void main() {
  // Simple 2d transform.
  vec2 position = inPosition;
  position *= scale;
  position += offset;

  texCoord0 = inTexCoord0;
  texCoord0 -= vec2(0.5, 0.5);
  texCoord0 = vec2(texCoord0.x * rotation.y + texCoord0.y * rotation.x,
                  texCoord0.y * rotation.y - texCoord0.x * rotation.x);
  texCoord0 += vec2(0.5, 0.5);

  gl_Position = vec4(position, 0.0, 1.0);
}
