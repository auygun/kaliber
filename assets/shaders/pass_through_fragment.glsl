precision mediump float;

uniform vec3      tileColor;
uniform sampler2D tileImage;

varying vec2      texCoord0;

void main() {
  gl_FragColor = texture2D(tileImage, texCoord0) * vec4(tileColor, 1.0);
}
