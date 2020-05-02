attribute vec2 in_position;
attribute vec2 in_tex_coord_0;

uniform vec2 scale;
uniform vec2 offset;
uniform vec2 pivot;
uniform vec2 rotation;
uniform vec2 tex_offset;
uniform vec2 tex_scale;
uniform mat4 projection;

varying vec2 tex_coord_0;

void main() {
  // Simple 2d transform.
  vec2 position = in_position;
  position *= scale;
  position += pivot;
  position = vec2(position.x * rotation.y + position.y * rotation.x,
                  position.y * rotation.y - position.x * rotation.x);
  position += offset - pivot;

  tex_coord_0 = (in_tex_coord_0 + tex_offset) * tex_scale;

  gl_Position = projection * vec4(position, 0.0, 1.0);
}
