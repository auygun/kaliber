attribute vec2 in_position;
attribute vec2 in_tex_coord_0;

uniform vec2 scale;
uniform mat4 projection;

varying vec2 tex_coord_0;

void main() {
  // Simple 2d transform.
  vec2 position = in_position;
  position *= scale;

  tex_coord_0 = in_tex_coord_0;

  gl_Position = projection * vec4(position, 0.0, 1.0);
}
