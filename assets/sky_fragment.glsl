precision highp float;

const vec2 resolution = vec2(2000.0, 2000.0);

uniform vec2 sky_offset;
uniform vec3 nebula_color;

float random (in vec2 st) {
  return fract(sin(dot(st.xy, vec2(12.9898, 78.233))) * 43758.5453123);
}

// Based on Morgan McGuire @morgan3d
// https://www.shadertoy.com/view/4dS3Wd
float noise (in vec2 st) {
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

vec2 rand2(vec2 p)
{
  p = vec2(dot(p, vec2(12.9898, 78.233)), dot(p, vec2(26.65125, 83.054543)));
  return fract(sin(p) * 43758.5453);
}

float rand(vec2 p)
{
  return fract(sin(dot(p.xy ,vec2(54.90898, 18.233))) * 4337.5453);
}

// https://www.shadertoy.com/view/4djGRh
float stars(in vec2 x, float numCells, float size, float br)
{
  vec2 n = x * numCells;
  vec2 f = floor(n);

  float d = 1.0e10;
  for (int i = -1; i <= 1; ++i)
  {
    for (int j = -1; j <= 1; ++j)
    {
      vec2 g = f + vec2(float(i), float(j));
      g = n - g - rand2(mod(g, numCells)) + rand(g);
      // Control size
      g *= 1.0 / (numCells * size);
      d = min(d, dot(g, g));
    }
  }

  return br * (smoothstep(0.95, 1.0, (1.0 - sqrt(d))));
}

void main() {
  vec3 result = vec3(0.);

  vec2 nebula_coord = gl_FragCoord.xy / resolution.xy;
  nebula_coord.x *= resolution.x / resolution.y;
  nebula_coord.y += sky_offset.y * 0.7;
  float c = 0.35 * noise(nebula_coord * 3.0) - 0.05;
  result += nebula_color * c;

  vec2 star_coord = gl_FragCoord.xy / resolution.y;
  result += stars(star_coord + sky_offset.xy, 8.0, 0.05, 1.0) * vec3(0.97, 0.74, 0.74);
  result += stars(star_coord + sky_offset.xy * 0.7, 16.0, 0.025, 0.5) * vec3(0.9, 0.9, 0.95);

  gl_FragColor = vec4(result, 1.0);
}
