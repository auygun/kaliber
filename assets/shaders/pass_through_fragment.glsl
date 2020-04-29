precision mediump float;

uniform vec3 tile_color;
uniform sampler2D tile_image;

varying vec2 tex_coord_0;

void main() {
  gl_FragColor = texture2D(tile_image, tex_coord_0) * vec4(tile_color, 1.0);
}
