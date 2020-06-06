// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file is based on the public domain code "stb_dxt.h" originally written
// by Fabian Giesen and Sean Barrett.
//
// The following changes were made:
// - Added support for ATC format.
// - Replaced the Principal Component Analysis based calculation to find the
//   initial base colors with a much simpler bounding box implementation for low
//   quality only.
// - Removed dithering support.
// - Some minor optimizations.
// - Reformatted the code (mainly with clang-format).
// - Swapped red and blue channels in the output to match Skia (Android only).

#include "dxt_encoder.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdlib>
#include <string.h>

#include "dxt_encoder_internals.h"

namespace {

struct TYPE_ATC_GENERIC : public TYPE_ATC {
  typedef TYPE_ATC BASE_TYPE;
  static const int kRemap[8];
  static const int kW1Table[4];
  static const int kProds[4];
};

struct TYPE_DXT_GENERIC : public TYPE_DXT {
  typedef TYPE_DXT BASE_TYPE;
  static const int kRemap[8];
  static const int kW1Table[4];
  static const int kProds[4];
};

const int TYPE_ATC_GENERIC::kRemap[8] =
    {0 << 30, 1 << 30, 0 << 30, 1 << 30, 2 << 30, 2 << 30, 3 << 30, 3 << 30};
const int TYPE_ATC_GENERIC::kW1Table[4] = {3, 2, 1, 0};
const int TYPE_ATC_GENERIC::kProds[4] = {0x090000,
                                         0x040102,
                                         0x010402,
                                         0x000900};

const int TYPE_DXT_GENERIC::kRemap[8] =
    {0 << 30, 2 << 30, 0 << 30, 2 << 30, 3 << 30, 3 << 30, 1 << 30, 1 << 30};
const int TYPE_DXT_GENERIC::kW1Table[4] = {3, 0, 2, 1};
const int TYPE_DXT_GENERIC::kProds[4] = {0x090000,
                                         0x000900,
                                         0x040102,
                                         0x010402};

inline int Mul8Bit(int a, int b) {
  int t = a * b + 128;
  return (t + (t >> 8)) >> 8;
}

// Linear interpolation at 1/3 point between a and b, using desired rounding
// type.
inline int Lerp13(int a, int b) {
  // Without rounding bias.
  // (2 * a + b) / 3;
  return ((2 * a + b) * 0xaaab) >> 17;
}

inline void Lerp13RGB(uint8_t* out, const uint8_t* p1, const uint8_t* p2) {
  out[0] = Lerp13(p1[0], p2[0]);
  out[1] = Lerp13(p1[1], p2[1]);
  out[2] = Lerp13(p1[2], p2[2]);
}

inline uint16_t As16Bit(int r, int g, int b) {
  return (Mul8Bit(r, 31) << 11) + (Mul8Bit(g, 63) << 5) + Mul8Bit(b, 31);
}

inline int sclamp(float y, int p0, int p1) {
  int x = static_cast<int>(y);
  if (x < p0) {
    return p0;
  }
  if (x > p1) {
    return p1;
  }
  return x;
}

// Take two 16-bit base colors and generate 4 32-bit RGBX colors where:
//  0 = c0
//  1 = c1
//  2 = (2 * c0 + c1) / 3
//  3 = (2 * c1 + c0) / 3
//
// The base colors are expanded by reusing the top bits at the end. That makes
// sure that white is still white after being quantized and converted back.
//
// params:
//  color   (output) 4 RGBA pixels.
//  c0      base color 0 (16 bit RGB)
//  c1      base color 1 (16 bit RGB)
inline void EvalColors(uint8_t* color, uint16_t c0, uint16_t c1) {
  // Expand the two base colors to 32-bit
  // From: [00000000][00000000][rrrrrggg][gggbbbbb]
  // To:   [00000000][bbbbbxxx][ggggggxx][rrrrrxxx]
  // Where x means repeat the upper bits for that color component.

  // Take shortcut if either color is zero. Both will never be zero.
  assert(c0 | c1);
  if (c0 && c1) {
    // Combine the two base colors into one register to allow operating on both
    // pixels at the same time.
    // [rrrrrggg][gggbbbbb][RRRRRGGG][GGGBBBBB]
    uint32_t c01 = c1 | (c0 << 16);

    // Mask out the red components and shift it down one channel to avoid some
    // shifts when combining the channels.
    // [00000000][rrrrr000][00000000][RRRRR000]
    uint32_t c01_r = (c01 & 0xf800f800) >> 8;
    // Extend to be 8-bit by reusing top bits at the end.
    // Note that we leave some extra garbage bits in the other channels, but
    // that's ok since we mask that off when we combine the different
    // components.
    // [00000000][rrrrrrrr][xx000000][RRRRRRRR]
    c01_r |= (c01_r >> 5);

    // Mask out the green components.
    // [00000ggg][ggg00000][00000GGG][GGG00000]
    uint32_t c01_g = c01 & 0x07e007e0;
    // Shift it into place and extend.
    // [gggggggg][xxxx0000][GGGGGGGG][xxxx0000]
    c01_g = ((c01_g << 5) | (c01_g >> 1));

    // Mask out the blue components.
    // [00000000][000bbbbb][00000000][000BBBBB]
    uint32_t c01_b = c01 & 0x001f001f;
    // Shift it into place and extend.
    // [bbbbbbbb][xx000000][BBBBBBBB][xx000000]
    c01_b = ((c01_b << 11) | (c01_b << 6));

    // Combine the components into base color 0.
    // Shift the components into place and mask of each channel.
    // [00000000][bbbbbbbb][gggggggg][rrrrrrrr]
    *reinterpret_cast<uint32_t*>(color + 0) = ((c01_r >> 16) & 0x000000ff) |
                                              ((c01_g >> 16) & 0x0000ff00) |
                                              ((c01_b >> 8) & 0x00ff0000);

    // Combine the components into base color 1.
    // [00000000][BBBBBBBB][GGGGGGGG][RRRRRRRR]
    *reinterpret_cast<uint32_t*>(color + 4) = (c01_r & 0x000000ff) |
                                              (c01_g & 0x0000ff00) |
                                              ((c01_b << 8) & 0x00ff0000);

    Lerp13RGB(color + 8, color, color + 4);
    Lerp13RGB(color + 12, color + 4, color);
  } else {
    // Combine the two base colors into one register, one of them will be zero.
    // [00000000][00000000][rrrrrggg][gggbbbbb]
    uint32_t c = c0 | c1;

    // Mask out the red components and shift it down one channel to avoid some
    // shifts when combining the channels.
    // [00000000][00000000][00000000][rrrrr000]
    uint32_t c_r = (c & 0xf800) >> 8;
    // Extend to be 8-bit by reusing top bits at the end.
    // [00000000][00000000][00000000][rrrrrrrr]
    c_r |= c_r >> 5;

    // Mask out the green components.
    // [00000000][00000000][00000ggg][ggg00000]
    uint32_t c_g = c & 0x07e0;
    // Shift it into place and extend. Then mask off garbage bits.
    // [00000000][00000000][gggggggg][xxxx0000]
    c_g = ((c_g << 5) | (c_g >> 1)) & 0x0000ff00;

    // Mask out the blue components.
    // [00000000][00000000][00000000][000bbbbb]
    uint32_t c_b = c & 0x001f;
    // Shift it into place and extend. Then mask off garbage bits.
    // [00000000][bbbbbbbb][xx000000][00000000]
    c_b = ((c_b << 19) | (c_b << 14)) & 0x00ff0000;

    size_t zero_offset = !!c0 * 4;
    size_t nonzero_offset = !!c1 * 4;

    // Combine the components into non zero base color.
    // [00000000][bbbbbbbb][gggggggg][rrrrrrrr]
    *reinterpret_cast<uint32_t*>(color + nonzero_offset) = c_r | c_g | c_b;

    // We already know that the other base color is zero.
    *reinterpret_cast<uint32_t*>(color + zero_offset) = 0;

    color[8 + nonzero_offset + 0] =
        (color[nonzero_offset + 0] * (2 * 0xaaab)) >> 17;
    color[8 + nonzero_offset + 1] =
        (color[nonzero_offset + 1] * (2 * 0xaaab)) >> 17;
    color[8 + nonzero_offset + 2] =
        (color[nonzero_offset + 2] * (2 * 0xaaab)) >> 17;

    color[8 + zero_offset + 0] = (color[nonzero_offset + 0] * 0xaaab) >> 17;
    color[8 + zero_offset + 1] = (color[nonzero_offset + 1] * 0xaaab) >> 17;
    color[8 + zero_offset + 2] = (color[nonzero_offset + 2] * 0xaaab) >> 17;
  }
}

// The color matching function.
template <typename T>
uint32_t MatchColorsBlock(const uint8_t* block, uint8_t* color) {
  int dirr = color[0 * 4 + 0] - color[1 * 4 + 0];
  int dirg = color[0 * 4 + 1] - color[1 * 4 + 1];
  int dirb = color[0 * 4 + 2] - color[1 * 4 + 2];

  int stops[4];
  for (int i = 0; i < 4; ++i) {
    stops[i] = color[i * 4 + 0] * dirr + color[i * 4 + 1] * dirg +
               color[i * 4 + 2] * dirb;
  }

  // Think of the colors as arranged on a line; project point onto that line,
  // then choose next color out of available ones. we compute the crossover
  // points for "best color in top half"/"best in bottom half" and then the same
  // inside that subinterval.
  //
  // Relying on this 1d approximation isn't always optimal in terms of euclidean
  // distance, but it's very close and a lot faster.
  // http://cbloomrants.blogspot.com/2008/12/12-08-08-dxtc-summary.html

  int c0_point = (stops[1] + stops[3]) >> 1;
  int half_point = (stops[3] + stops[2]) >> 1;
  int c3_point = (stops[2] + stops[0]) >> 1;

  uint32_t mask = 0;
  for (int i = 0; i < 16; i++) {
    int dot = block[i * 4 + 0] * dirr + block[i * 4 + 1] * dirg +
              block[i * 4 + 2] * dirb;

    int bits = ((dot < half_point) ? 4 : 0) | ((dot < c0_point) ? 2 : 0) |
               ((dot < c3_point) ? 1 : 0);

    mask >>= 2;
    mask |= T::kRemap[bits];
  }

  return mask;
}

void GetBaseColors(const uint8_t* block,
                   int v_r,
                   int v_g,
                   int v_b,
                   uint16_t* pmax16,
                   uint16_t* pmin16) {
  // Pick colors at extreme points.
#ifdef VERIFY_RESULTS
  // Rewritten to match the SIMD implementation, not as efficient.
  int dots[16];
  for (int i = 0; i < 16; ++i) {
    dots[i] = block[i * 4 + 0] * v_r + block[i * 4 + 1] * v_g +
              block[i * 4 + 2] * v_b;
  }
  int max_dot = dots[0];
  int min_dot = dots[0];
  for (int i = 1; i < 16; ++i) {
    if (dots[i] > max_dot)
      max_dot = dots[i];
    if (dots[i] < min_dot)
      min_dot = dots[i];
  }
  uint32_t max_pixels[16];
  uint32_t min_pixels[16];
  for (int i = 0; i < 16; ++i) {
    const uint32_t* p = reinterpret_cast<const uint32_t*>(block) + i;
    max_pixels[i] = (dots[i] == max_dot) ? *p : 0;
    min_pixels[i] = (dots[i] == min_dot) ? *p : 0;
  }
  uint32_t max_pixel = max_pixels[0];
  uint32_t min_pixel = min_pixels[0];
  for (int i = 1; i < 16; ++i) {
    if (max_pixels[i] > max_pixel) {
      max_pixel = max_pixels[i];
    }
    if (min_pixels[i] > min_pixel) {
      min_pixel = min_pixels[i];
    }
  }
  uint8_t* maxp = reinterpret_cast<uint8_t*>(&max_pixel);
  uint8_t* minp = reinterpret_cast<uint8_t*>(&min_pixel);
#else
  int mind = 0x7fffffff;
  int maxd = -0x7fffffff;
  const uint8_t* minp = block;
  const uint8_t* maxp = block;
  for (int i = 0; i < 16; ++i) {
    int dot = block[i * 4 + 0] * v_r + block[i * 4 + 1] * v_g +
              block[i * 4 + 2] * v_b;

    if (dot < mind) {
      mind = dot;
      minp = block + i * 4;
    }

    if (dot > maxd) {
      maxd = dot;
      maxp = block + i * 4;
    }
  }
#endif

  *pmax16 = As16Bit(maxp[0], maxp[1], maxp[2]);
  *pmin16 = As16Bit(minp[0], minp[1], minp[2]);
}

// Figure out the two base colors to use from a block of 16 pixels
// by Primary Component Analysis and map along principal axis.
//
// params:
//  block   16 32-bit RGBX colors.
//  pmax16  (output) base color 0 (minimum value), 16-bit RGB
//  pmin16  (output) base color 1 (maximum value), 16-bit RGB
void OptimizeColorsBlock(const uint8_t* block,
                         uint16_t* pmax16,
                         uint16_t* pmin16) {
  // Determine color distribution.
  int mu[3];
  int min[3];
  int max[3];
  for (int ch = 0; ch < 3; ++ch) {
    const uint8_t* bp = block + ch;
    int muv = bp[0];
    int minv = muv;
    int maxv = muv;
    for (int i = 4; i < 64; i += 4) {
      int pixel = bp[i];
      muv += pixel;
      if (pixel < minv) {
        minv = pixel;
      } else if (pixel > maxv) {
        maxv = pixel;
      }
    }

    mu[ch] = (muv + 8) >> 4;
    min[ch] = minv;
    max[ch] = maxv;
  }

  // Determine covariance matrix.
  int cov[6] = {0, 0, 0, 0, 0, 0};
  for (int i = 0; i < 16; ++i) {
    int r = block[i * 4 + 0] - mu[0];
    int g = block[i * 4 + 1] - mu[1];
    int b = block[i * 4 + 2] - mu[2];

    cov[0] += r * r;
    cov[1] += r * g;
    cov[2] += r * b;
    cov[3] += g * g;
    cov[4] += g * b;
    cov[5] += b * b;
  }

  // Convert covariance matrix to float, find principal axis via power iter.
  float covf[6];
  for (int i = 0; i < 6; ++i) {
    covf[i] = cov[i] / 255.0f;
  }

  float vfr = static_cast<float>(max[0] - min[0]);
  float vfg = static_cast<float>(max[1] - min[1]);
  float vfb = static_cast<float>(max[2] - min[2]);

  // Iterate to the power of 4.
  for (int iter = 0; iter < 4; ++iter) {
    float r = vfr * covf[0] + vfg * covf[1] + vfb * covf[2];
    float g = vfr * covf[1] + vfg * covf[3] + vfb * covf[4];
    float b = vfr * covf[2] + vfg * covf[4] + vfb * covf[5];

    vfr = r;
    vfg = g;
    vfb = b;
  }

  double magn = std::abs(vfr);
  double mag_g = std::abs(vfg);
  double mag_b = std::abs(vfb);
  if (mag_g > magn) {
    magn = mag_g;
  }
  if (mag_b > magn) {
    magn = mag_b;
  }

  int v_r = 299;  // JPEG YCbCr luma coefs, scaled by 1000.
  int v_g = 587;
  int v_b = 114;
  if (magn >= 4.0f) {  // Too small, default to luminance.
    magn = 512.0 / magn;
    v_r = static_cast<int>(vfr * magn);
    v_g = static_cast<int>(vfg * magn);
    v_b = static_cast<int>(vfb * magn);
  }

  GetBaseColors(block, v_r, v_g, v_b, pmax16, pmin16);
}

// Figure out the two base colors simply using a direction vector between min
// and max colors.
//
// params:
//  block   16 32-bit RGBX colors.
//  pmax16  (output) base color 0 (minimum value), 16-bit RGB
//  pmin16  (output) base color 1 (maximum value), 16-bit RGB
void GetApproximateBaseColors(const uint8_t* block,
                              uint16_t* pmax16,
                              uint16_t* pmin16) {
  uint8_t dir[3];
  for (int ch = 0; ch < 3; ++ch) {
    const uint8_t* bp = block + ch;
    uint8_t minv = bp[0];
    uint8_t maxv = bp[0];
    for (int i = 4; i < 64; i += 4) {
      uint8_t pixel = bp[i];
      if (pixel < minv) {
        minv = pixel;
      } else if (pixel > maxv) {
        maxv = pixel;
      }
    }

    dir[ch] = maxv - minv;
  }

  GetBaseColors(block, dir[0], dir[1], dir[2], pmax16, pmin16);
}

// The refinement function.
// Tries to optimize colors to suit block contents better.
// (By solving a least squares system via normal equations+Cramer's rule)
//
// params:
//  block   16 32-bit RGBX colors.
//  pmax16  (output) base color 0 (minimum value), 16-bit RGB
//  pmin16  (output) base color 1 (maximum value), 16-bit RGB
//  mask    16 2-bit color indices.
template <typename T>
int RefineBlock(const uint8_t* block,
                uint16_t* pmax16,
                uint16_t* pmin16,
                uint32_t mask) {
  uint16_t min16 = 0;
  uint16_t max16 = 0;
  if ((mask ^ (mask << 2)) < 4) {  // All pixels have the same index?
    // Yes, linear system would be singular; solve using optimal
    // single-color match on average color.
    int r = 8;
    int g = 8;
    int b = 8;
    for (int i = 0; i < 16; ++i) {
      r += block[i * 4 + 0];
      g += block[i * 4 + 1];
      b += block[i * 4 + 2];
    }

    r >>= 4;
    g >>= 4;
    b >>= 4;

    max16 = Match8BitColorMax<typename T::BASE_TYPE>(r, g, b);
    min16 = Match8BitColorMin<typename T::BASE_TYPE>(r, g, b);
  } else {
    int at1_r = 0;
    int at1_g = 0;
    int at1_b = 0;
    int at2_r = 0;
    int at2_g = 0;
    int at2_b = 0;
    int akku = 0;
    uint32_t cm = mask;
    for (int i = 0; i < 16; ++i, cm >>= 2) {
      int step = cm & 3;

      int w1 = T::kW1Table[step];
      int r = block[i * 4 + 0];
      int g = block[i * 4 + 1];
      int b = block[i * 4 + 2];

      // Some magic to save a lot of multiplies in the accumulating loop...
      // (Precomputed products of weights for least squares system, accumulated
      // inside one 32-bit register.)
      akku += T::kProds[step];
      at1_r += w1 * r;
      at1_g += w1 * g;
      at1_b += w1 * b;
      at2_r += r;
      at2_g += g;
      at2_b += b;
    }

    at2_r = 3 * at2_r - at1_r;
    at2_g = 3 * at2_g - at1_g;
    at2_b = 3 * at2_b - at1_b;

    // Extract solutions and decide solvability.
    int xx = akku >> 16;
    int yy = (akku >> 8) & 0xff;
    int xy = (akku >> 0) & 0xff;

    float frb = 3.0f * 31.0f / 255.0f / (xx * yy - xy * xy);
    float fg = frb * 63.0f / 31.0f;

    // Solve.
    max16 = sclamp((at1_r * yy - at2_r * xy) * frb + 0.5f, 0, 31) << 11;
    max16 |= sclamp((at1_g * yy - at2_g * xy) * fg + 0.5f, 0, 63) << 5;
    max16 |= sclamp((at1_b * yy - at2_b * xy) * frb + 0.5f, 0, 31) << 0;

    min16 = sclamp((at2_r * xx - at1_r * xy) * frb + 0.5f, 0, 31) << 11;
    min16 |= sclamp((at2_g * xx - at1_g * xy) * fg + 0.5f, 0, 63) << 5;
    min16 |= sclamp((at2_b * xx - at1_b * xy) * frb + 0.5f, 0, 31) << 0;
  }

  uint16_t oldMin = *pmin16;
  uint16_t oldMax = *pmax16;
  *pmin16 = min16;
  *pmax16 = max16;
  return oldMin != min16 || oldMax != max16;
}

// Color block compression.
template <typename T>
void CompressColorBlock(uint8_t* dst,
                        const uint8_t* block,
                        TextureCompressor::Quality quality) {
  // Check if block is constant.
  int i = 1;
  uint32_t first_pixel =
      reinterpret_cast<const uint32_t*>(block)[0] & 0x00ffffff;
  for (; i < 16; ++i) {
    if ((reinterpret_cast<const uint32_t*>(block)[i] & 0x00ffffff) !=
        first_pixel) {
      break;
    }
  }

  uint32_t mask = 0;
  uint16_t max16 = 0;
  uint16_t min16 = 0;
  if (i == 16) {  // Constant color
    int r = block[0];
    int g = block[1];
    int b = block[2];
    max16 = Match8BitColorMax<typename T::BASE_TYPE>(r, g, b);
    min16 = Match8BitColorMin<typename T::BASE_TYPE>(r, g, b);
    mask = T::kConstantColorIndices;
  } else {
    if (quality == TextureCompressor::kQualityLow) {
      GetApproximateBaseColors(block, &max16, &min16);
    } else {
      // Do Primary Component Analysis and map along principal axis.
      OptimizeColorsBlock(block, &max16, &min16);
    }

    if (max16 != min16) {
      uint8_t color[4 * 4];
      EvalColors(color, max16, min16);
      mask = MatchColorsBlock<T>(block, color);
    }

    if (quality == TextureCompressor::kQualityHigh) {
      // Refine (multiple times if requested).
      for (int i = 0; i < kNumRefinements; ++i) {
        uint32_t last_mask = mask;

        if (RefineBlock<T>(block, &max16, &min16, mask)) {
          if (max16 != min16) {
            uint8_t color[4 * 4];
            EvalColors(color, max16, min16);
            mask = MatchColorsBlock<T>(block, color);
          } else {
            mask = 0;
            break;
          }
        }

        if (mask == last_mask) {
          break;
        }
      }
    }
  }

  FormatFixup<typename T::BASE_TYPE>(&max16, &min16, &mask);

  uint32_t* dst32 = reinterpret_cast<uint32_t*>(dst);
  dst32[0] = max16 | (min16 << 16);
  dst32[1] = mask;
}

// Alpha block compression.
void CompressAlphaBlock(uint8_t* dst, const uint8_t* src) {
  // Find min/max alpha.
  int mn = src[3];
  int mx = mn;
  for (int i = 1; i < 16; ++i) {
    int alpha = src[i * 4 + 3];
    if (alpha < mn) {
      mn = alpha;
    } else if (alpha > mx) {
      mx = alpha;
    }
  }

  // Encode them.
  dst[0] = mx;
  dst[1] = mn;
  dst += 2;

  if (mx == mn) {
    memset(dst, 0, 6);
  } else {
    // Determine bias and emit color indices.
    // Given the choice of mx/mn, these indices are optimal:
    // http://fgiesen.wordpress.com/2009/12/15/dxt5-alpha-block-index-determination/
    int dist = mx - mn;
    int dist4 = dist * 4;
    int dist2 = dist * 2;
    int bias = (dist < 8) ? (dist - 1) : (dist / 2 + 2);
    bias -= mn * 7;
    int bits = 0;
    int mask = 0;

    for (int i = 0; i < 16; ++i) {
      int a = src[i * 4 + 3] * 7 + bias;

      // Select index. this is a "linear scale" lerp factor between 0 (val=min)
      // and 7 (val=max).
      int t = (a >= dist4) ? -1 : 0;
      int ind = t & 4;
      a -= dist4 & t;

      t = (a >= dist2) ? -1 : 0;
      ind += t & 2;
      a -= dist2 & t;

      ind += (a >= dist);

      // Turn linear scale into DXT index (0/1 are extremal pts).
      ind = -ind & 7;
      ind ^= (2 > ind);

      // Write index.
      mask |= ind << bits;
      if ((bits += 3) >= 8) {
        *dst++ = mask;
        mask >>= 8;
        bits -= 8;
      }
    }
  }
}

// Extract up to a 4x4 block of pixels and "de-swizzle" them into 16x1.
// If 'num_columns' or 'num_rows' are less than 4 then it fills out the rest
// of the block by taking a copy of the last valid column or row.
void ExtractBlock(uint8_t* dst,
                  const uint8_t* src,
                  int num_columns,
                  int num_rows,
                  int width) {
  uint32_t* wdst = reinterpret_cast<uint32_t*>(dst);
  const uint32_t* wsrc = reinterpret_cast<const uint32_t*>(src);

  for (int y = 0; y < num_rows; ++y) {
    for (int x = 0; x < num_columns; ++x)
      *wdst++ = *wsrc++;
    // Fill remaining columns with values from last column.
    uint32_t* padding = wdst - 1;
    for (int x = num_columns; x < 4; ++x)
      *wdst++ = *padding;
    wsrc += width - num_columns;
  }

  // Fill remaining rows with values from last row.
  const uint32_t* last_row = wdst - 4;
  for (int y = num_rows; y < 4; ++y) {
    const uint32_t* padding = last_row;
    for (int x = 0; x < 4; ++x)
      *wdst++ = *padding++;
  }
}

}  // namespace

void CompressATC(const uint8_t* src,
                 uint8_t* dst,
                 int width,
                 int height,
                 bool opaque,
                 TextureCompressor::Quality quality) {
  assert(quality >= TextureCompressor::kQualityLow &&
         quality <= TextureCompressor::kQualityHigh);

  // The format works on blocks of 4x4 pixels.
  // If the size is misaligned then we need to be careful when extracting the
  // block of source texels.

  for (int y = 0; y < height; y += 4, src += width * 4 * 4) {
    // Figure out if we need to skip the last few rows or not.
    int num_rows = std::min(height - y, 4);

    for (int x = 0; x < width; x += 4) {
      // Figure out of we need to skip the last few columns or not.
      int num_columns = std::min(width - x, 4);

      uint8_t block[64];
      ExtractBlock(block, src + x * 4, num_columns, num_rows, width);

      if (!opaque) {
        CompressAlphaBlock(dst, block);
        dst += 8;
      }

      CompressColorBlock<TYPE_ATC_GENERIC>(dst, block, quality);
      dst += 8;
    }
  }
}

void CompressDXT(const uint8_t* src,
                 uint8_t* dst,
                 int width,
                 int height,
                 bool opaque,
                 TextureCompressor::Quality quality) {
  assert(quality >= TextureCompressor::kQualityLow &&
         quality <= TextureCompressor::kQualityHigh);

  for (int y = 0; y < height; y += 4, src += width * 4 * 4) {
    int num_rows = std::min(height - y, 4);

    for (int x = 0; x < width; x += 4) {
      int num_columns = std::min(width - x, 4);

      uint8_t block[64];
      ExtractBlock(block, src + x * 4, num_columns, num_rows, width);

      if (!opaque) {
        CompressAlphaBlock(dst, block);
        dst += 8;
      }

      CompressColorBlock<TYPE_DXT_GENERIC>(dst, block, quality);
      dst += 8;
    }
  }
}
