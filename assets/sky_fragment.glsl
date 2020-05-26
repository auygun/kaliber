precision mediump float;

uniform highp vec2 sky_offset;
uniform vec3 nebula_color;

varying highp vec2 tex_coord_0;

float random(highp vec2 p) {
  return fract(sin(dot(p, vec2(54.90898, 18.233))) * 4337.5453);
}

float nebula(in highp vec2 p) {
  highp vec2 i = floor(p);
  highp vec2 f = fract(p);

  float a = random(i);
  float b = random(i + vec2(1.0, 0.0));
  float c = random(i + vec2(0.0, 1.0));
  float d = random(i + vec2(1.0, 1.0));

  vec2 u = f * f * (3.0 - 2.0 * f);

  return mix(a, b, u.x) +
      (c - a)* u.y * (1.0 - u.x) +
      (d - b) * u.x * u.y;
}

float stars(in highp vec2 p, float num_cells, float size) {
  highp vec2 n = p * num_cells;
  highp vec2 i = floor(n);

  vec2 a = i;
  vec2 b = i + vec2(1.0, 0.0);
  vec2 c = i + vec2(0.0, 1.0);
  vec2 d = i + vec2(1.0, 1.0);

  float w = 1.0 / (num_cells * size);
  a = (n - a - random(a)) * w;
  b = (n - b - random(b)) * w;
  c = (n - c - random(c)) * w;
  d = (n - d - random(d)) * w;

  float e = dot(a, a);
  e = min(e, dot(b, b));
  e = min(e, dot(c, c));
  e = min(e, dot(d, d));

  return smoothstep(0.95, 1.0, (1.0 - sqrt(e)));
}

void main() {
  highp vec2 layer1_coord = tex_coord_0 + sky_offset;
  highp vec2 layer2_coord = tex_coord_0 + sky_offset  * 0.7;
  vec3 result = vec3(0.);

  float c = nebula(layer2_coord * 3.0) * 0.35 - 0.05;
  result += nebula_color * floor(c * 60.0) / 60.0;

  c = stars(layer1_coord, 8.0, 0.05);
  result += vec3(0.97, 0.74, 0.74) * c;

  c = stars(layer2_coord, 16.0, 0.025) * 0.5;
  result += vec3(0.9, 0.9, 0.95) * c;

  gl_FragColor = vec4(result, 1.0);
}
