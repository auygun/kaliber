precision mediump float;

const float resolution = 2000.0;

uniform highp vec2 sky_offset;
uniform vec3 nebula_color;

float random(highp vec2 p)
{
  return fract(sin(dot(p.xy ,vec2(54.90898, 18.233))) * 4337.5453);
}

vec2 random2(vec2 p)
{
  p = vec2(dot(p, vec2(12.9898, 78.233)), dot(p, vec2(26.65125, 83.054543)));
  return fract(sin(p) * 43758.5453);
}

float nebula(in vec2 st) {
  vec2 i = floor(st);
  vec2 f = fract(st);

  // Four corners in 2D of a tile
  float a = random(i);
  float b = random(i + vec2(1.0, 0.0));
  float c = random(i + vec2(0.0, 1.0));
  float d = random(i + vec2(1.0, 1.0));

  vec2 u = f * f * (3.0 - 2.0 * f);

  return mix(a, b, u.x) +
      (c - a)* u.y * (1.0 - u.x) +
      (d - b) * u.x * u.y;
}

float stars(in vec2 x, float numCells, float size, float br)
{
  vec2 n = x * numCells;
  vec2 f = floor(n);

  vec2 a = f;
  a = n - a - random2(mod(a, numCells)) + random(a);
  a *= 1.0 / (numCells * size);
  float e = dot(a, a);

  vec2 b = f + vec2(1.0, 0.0);
  b = n - b - random2(mod(b, numCells)) + random(b);
  b *= 1.0 / (numCells * size);
  e = min(e, dot(b, b));

  vec2 c = f + vec2(0.0, 1.0);
  c = n - c - random2(mod(c, numCells)) + random(c);
  c *= 1.0 / (numCells * size);
  e = min(e, dot(c, c));

  vec2 d = f + vec2(1.0, 1.0);
  d = n - d - random2(mod(d, numCells)) + random(d);
  d *= 1.0 / (numCells * size);
  e = min(e, dot(d, d));

  return br * (smoothstep(0.95, 1.0, (1.0 - sqrt(e))));
}

void main() {
  highp vec2 sky_coord = gl_FragCoord.xy / resolution;
  vec3 result = vec3(0.);

  float c = 0.35 * nebula((sky_coord + sky_offset.xy * 0.7) * 3.0) - 0.05;
  result += nebula_color * floor(c * 60.0) / 60.0;

  result += stars(sky_coord + sky_offset.xy, 8.0, 0.05, 1.0) * vec3(0.97, 0.74, 0.74);
  result += stars(sky_coord + sky_offset.xy * 0.7, 16.0, 0.025, 0.5) * vec3(0.9, 0.9, 0.95);

  gl_FragColor = vec4(result, 1.0);
}
