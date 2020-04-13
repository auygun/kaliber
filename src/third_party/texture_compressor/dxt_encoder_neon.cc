// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// See the links below for detailed descriptions of the algorithms used.
// http://cbloomrants.blogspot.se/2008/12/12-08-08-dxtc-summary.html
// http://fgiesen.wordpress.com/2009/12/15/dxt5-alpha-block-index-determination

#include "dxt_encoder_neon.h"

#include <arm_neon.h>

#include <algorithm>
#include <cassert>

#include "dxt_encoder_internals.h"

#if defined(__GNUC__)
#define ALWAYS_INLINE __attribute__((always_inline))
#else
#define ALWAYS_INLINE inline
#endif

#define ALIGNAS(X) __attribute__ ((aligned (X)))

namespace {

struct TYPE_ATC_NEON : public TYPE_ATC {
  typedef TYPE_ATC BASE_TYPE;
  static const uint8x8_t kRemap;
  static const uint64_t kProds[3];
};

struct TYPE_DXT_NEON : public TYPE_DXT {
  typedef TYPE_DXT BASE_TYPE;
  static const uint8x8_t kRemap;
  static const int8x8_t kW1Table;
  static const uint64_t kProds[3];
};

const uint8x8_t TYPE_ATC_NEON::kRemap = {0, 1, 0, 1, 2, 2, 3, 3};
const uint64_t TYPE_ATC_NEON::kProds[3] = {0x00010409, 0x09040100, 0x00020200};

const uint8x8_t TYPE_DXT_NEON::kRemap = {0, 2, 0, 2, 3, 3, 1, 1};
const int8x8_t TYPE_DXT_NEON::kW1Table = {3, 0, 2, 1, 0, 0, 0, 0};
const uint64_t TYPE_DXT_NEON::kProds[3] = {0x01040009, 0x04010900, 0x02020000};

template <typename T>
ALWAYS_INLINE int8x16_t DoW1TableLookup(uint8x16_t indices);

template <>
ALWAYS_INLINE int8x16_t DoW1TableLookup<TYPE_ATC_NEON>(uint8x16_t indices) {
  // Take a shortcut for ATC which gives the same result as the table lookup.
  // {0, 1, 2, 3} -> {3, 2, 1, 0}
  return veorq_s8(vreinterpretq_s8_u8(indices), vdupq_n_s8(3));
}

template <>
ALWAYS_INLINE int8x16_t DoW1TableLookup<TYPE_DXT_NEON>(uint8x16_t indices) {
  // Do table lookup for each color index.
  return vcombine_s8(vtbl1_s8(TYPE_DXT_NEON::kW1Table,
                              vreinterpret_s8_u8(vget_low_u8(indices))),
                     vtbl1_s8(TYPE_DXT_NEON::kW1Table,
                              vreinterpret_s8_u8(vget_high_u8(indices))));
}

// Returns max and min base color components matching the given 8-bit color
// component when solved via linear interpolation. Output format differs for ATC
// and DXT. See explicitly instantiated template functions below.
template <typename T>
ALWAYS_INLINE uint16_t Match8BitComponentMax(int g);
template <typename T>
ALWAYS_INLINE uint16_t Match8BitComponentMin(int g);

template <>
ALWAYS_INLINE uint16_t Match8BitComponentMax<TYPE_ATC>(int g) {
  return kDXTConstantColors56[g][0] << 1;
}

template <>
ALWAYS_INLINE uint16_t Match8BitComponentMin<TYPE_ATC>(int g) {
  return kDXTConstantColors56[g][1];
}

template <>
ALWAYS_INLINE uint16_t Match8BitComponentMax<TYPE_DXT>(int g) {
  return kDXTConstantColors66[g][0];
}

template <>
ALWAYS_INLINE uint16_t Match8BitComponentMin<TYPE_DXT>(int g) {
  return kDXTConstantColors66[g][1];
}

// This converts the output data to either ATC or DXT format.
// See explicitly instantiated template functions below.
template <typename T>
ALWAYS_INLINE void FormatFixupIdx(uint16x4_t* base_colors, uint64x1_t* indices);

template <>
ALWAYS_INLINE void FormatFixupIdx<TYPE_ATC_NEON>(uint16x4_t* base_colors,
                                                 uint64x1_t* indices) {
  // First color in ATC format is 555.
  *base_colors = vorr_u16(
      vand_u16(*base_colors, vreinterpret_u16_u64(vdup_n_u64(0xffff001f))),
      vshr_n_u16(
          vand_u16(*base_colors, vreinterpret_u16_u64(vdup_n_u64(0x0000ffC0))),
          1));
}

template <>
ALWAYS_INLINE void FormatFixupIdx<TYPE_DXT_NEON>(uint16x4_t* base_colors,
                                                 uint64x1_t* indices) {
  // Swap min/max colors if necessary.
  uint16x4_t max = vdup_lane_u16(*base_colors, 0);
  uint16x4_t min = vdup_lane_u16(*base_colors, 1);
  uint16x4_t cmp = vclt_u16(max, min);
  *base_colors =
      vorr_u16(vand_u16(vbsl_u16(cmp, min, max),
                        vreinterpret_u16_u64(vdup_n_u64(0x0000ffff))),
               vand_u16(vbsl_u16(cmp, max, min),
                        vreinterpret_u16_u64(vdup_n_u64(0xffff0000))));
  *indices = vbsl_u64(vreinterpret_u64_u16(cmp),
                      veor_u64(*indices, vdup_n_u64(0x55555555)), *indices);
}

// Check if all the 8 bits elements in the given quad register are equal.
ALWAYS_INLINE bool ElementsEqual(uint8x16_t elements) {
  uint8x16_t first = vdupq_lane_u8(vget_low_u8(elements), 0);
  uint8x16_t eq = vceqq_u8(elements, first);
  uint8x8_t tst = vand_u8(vget_low_u8(eq), vget_high_u8(eq));
  return vget_lane_u64(vreinterpret_u64_u8(tst), 0) == 0xffffffffffffffff;
}

ALWAYS_INLINE bool Equal(uint8x16_t e1, uint8x16_t e2) {
  uint8x16_t eq = vceqq_u8(e1, e2);
  uint8x8_t tst = vand_u8(vget_low_u8(eq), vget_high_u8(eq));
  return vget_lane_u64(vreinterpret_u64_u8(tst), 0) == 0xffffffffffffffff;
}

ALWAYS_INLINE bool Equal(uint16x8_t e1, uint16x8_t e2) {
  uint16x8_t eq = vceqq_u16(e1, e2);
  uint16x4_t tst = vand_u16(vget_low_u16(eq), vget_high_u16(eq));
  return vget_lane_u64(vreinterpret_u64_u16(tst), 0) == 0xffffffffffffffff;
}

ALWAYS_INLINE bool Equal(uint16x4_t e1, uint16x4_t e2) {
  uint16x4_t eq = vceq_u16(e1, e2);
  return vget_lane_u64(vreinterpret_u64_u16(eq), 0) == 0xffffffffffffffff;
}

ALWAYS_INLINE int16x8x2_t ExpandRGBATo16(const uint8x16_t& channel) {
  int16x8x2_t result;
  result.val[0] = vreinterpretq_s16_u16(vmovl_u8(vget_low_u8(channel)));
  result.val[1] = vreinterpretq_s16_u16(vmovl_u8(vget_high_u8(channel)));
  return result;
}

ALWAYS_INLINE int32x4x4_t ExpandRGBATo32(const uint8x16_t& channel) {
  uint16x8_t lo = vmovl_u8(vget_low_u8(channel));
  uint16x8_t hi = vmovl_u8(vget_high_u8(channel));
  int32x4x4_t result;
  result.val[0] = vreinterpretq_s32_u32(vmovl_u16(vget_low_u16(lo)));
  result.val[1] = vreinterpretq_s32_u32(vmovl_u16(vget_high_u16(lo)));
  result.val[2] = vreinterpretq_s32_u32(vmovl_u16(vget_low_u16(hi)));
  result.val[3] = vreinterpretq_s32_u32(vmovl_u16(vget_high_u16(hi)));
  return result;
}

// NEON doesn't have support for division.
// Instead it's recommended to use Newton-Raphson refinement to get a close
// approximation.
template <int REFINEMENT_STEPS>
ALWAYS_INLINE float32x4_t Divide(float32x4_t a, float32x4_t b) {
#ifdef VERIFY_RESULTS
  ALIGNAS(8) float a_[4];
  ALIGNAS(8) float b_[4];
  vst1q_f32(a_, a);
  vst1q_f32(b_, b);
  for (int i = 0; i < 4; ++i)
    a_[i] /= b_[i];
  return vld1q_f32(a_);
#else
  // Get an initial estimate of 1/b.
  float32x4_t reciprocal = vrecpeq_f32(b);
  // Use a number of Newton-Raphson steps to refine the estimate.
  for (int i = 0; i < REFINEMENT_STEPS; ++i)
    reciprocal = vmulq_f32(vrecpsq_f32(b, reciprocal), reciprocal);
  // Calculate the final estimate.
  return vmulq_f32(a, reciprocal);
#endif
}

namespace vec_ops {

struct Max {
  ALWAYS_INLINE int32x4_t Calc(int32x4_t a, int32x4_t b) {
    return vmaxq_s32(a, b);
  }

  ALWAYS_INLINE uint32x4_t Calc(uint32x4_t a, uint32x4_t b) {
    return vmaxq_u32(a, b);
  }

  ALWAYS_INLINE uint8x8_t Fold(uint8x8_t a, uint8x8_t b) {
    return vpmax_u8(a, b);
  }

  ALWAYS_INLINE int32x2_t Fold(int32x2_t a, int32x2_t b) {
    return vpmax_s32(a, b);
  }

  ALWAYS_INLINE uint32x2_t Fold(uint32x2_t a, uint32x2_t b) {
    return vpmax_u32(a, b);
  }
};

struct Min {
  ALWAYS_INLINE int32x4_t Calc(int32x4_t a, int32x4_t b) {
    return vminq_s32(a, b);
  }

  ALWAYS_INLINE uint32x4_t Calc(uint32x4_t a, uint32x4_t b) {
    return vminq_u32(a, b);
  }

  ALWAYS_INLINE uint8x8_t Fold(uint8x8_t a, uint8x8_t b) {
    return vpmin_u8(a, b);
  }

  ALWAYS_INLINE int32x2_t Fold(int32x2_t a, int32x2_t b) {
    return vpmin_s32(a, b);
  }

  ALWAYS_INLINE uint32x2_t Fold(uint32x2_t a, uint32x2_t b) {
    return vpmin_u32(a, b);
  }
};

}  // namespace vec_ops

template <typename Operator>
ALWAYS_INLINE uint8x8_t FoldRGBA(const uint8x16x4_t& src) {
  Operator op;

  // Fold each adjacent pair.
  uint8x8_t r = op.Fold(vget_low_u8(src.val[0]), vget_high_u8(src.val[0]));
  uint8x8_t g = op.Fold(vget_low_u8(src.val[1]), vget_high_u8(src.val[1]));
  uint8x8_t b = op.Fold(vget_low_u8(src.val[2]), vget_high_u8(src.val[2]));
  uint8x8_t a = op.Fold(vget_low_u8(src.val[3]), vget_high_u8(src.val[3]));

  // Do both red and green channels at the same time.
  uint8x8_t rg = op.Fold(r, g);

  // Do both blue and alpha channels at the same time.
  uint8x8_t ba = op.Fold(b, a);

  // Do all the channels at the same time.
  uint8x8_t rgba = op.Fold(rg, ba);

  // Finally, we need to pad it to get the final reduction.
  return op.Fold(rgba, rgba);
}

template <typename Operator>
ALWAYS_INLINE int32x2_t Fold(const int32x4x4_t& src) {
  Operator op;

  int32x4_t fold0 = op.Calc(src.val[0], src.val[1]);
  int32x4_t fold1 = op.Calc(src.val[2], src.val[3]);
  int32x4_t fold01 = op.Calc(fold0, fold1);
  int32x2_t fold0123 = op.Fold(vget_low_s32(fold01), vget_high_s32(fold01));
  return op.Fold(fold0123, vdup_n_s32(0));
}

template <typename Operator>
ALWAYS_INLINE uint32x2_t Fold(const uint32x4x4_t& src) {
  Operator op;

  uint32x4_t fold0 = op.Calc(src.val[0], src.val[1]);
  uint32x4_t fold1 = op.Calc(src.val[2], src.val[3]);
  uint32x4_t fold01 = op.Calc(fold0, fold1);
  uint32x2_t fold0123 = op.Fold(vget_low_u32(fold01), vget_high_u32(fold01));
  return op.Fold(fold0123, vdup_n_u32(0));
}

template <typename Operator>
ALWAYS_INLINE int32x4_t FoldDup(const int32x4x4_t& src) {
  return vdupq_lane_s32(Fold<Operator>(src), 0);
}

ALWAYS_INLINE uint16x4_t SumRGB(const uint8x16x4_t& src) {
  // Add up all red values for 16 pixels.
  uint16x8_t r = vpaddlq_u8(src.val[0]);
  uint16x4_t r2 = vpadd_u16(vget_low_u16(r), vget_high_u16(r));

  // Add up all green values for 16 pixels.
  uint16x8_t g = vpaddlq_u8(src.val[1]);
  uint16x4_t g2 = vpadd_u16(vget_low_u16(g), vget_high_u16(g));

  uint16x4_t rg = vpadd_u16(r2, g2);

  // Add up all blue values for 16 pixels.
  uint16x8_t b = vpaddlq_u8(src.val[2]);
  uint16x4_t b2 = vpadd_u16(vget_low_u16(b), vget_high_u16(b));

  uint16x4_t ba = vpadd_u16(b2, vdup_n_u16(0));

  return vpadd_u16(rg, ba);
}

ALWAYS_INLINE int32x4_t SumRGB(const int16x8x4_t& src) {
  // Add up all red values for 8 pixels.
  int32x4_t r = vpaddlq_s16(src.val[0]);
  int32x2_t r2 = vpadd_s32(vget_low_s32(r), vget_high_s32(r));

  // Add up all green values for 8 pixels.
  int32x4_t g = vpaddlq_s16(src.val[1]);
  int32x2_t g2 = vpadd_s32(vget_low_s32(g), vget_high_s32(g));

  int32x2_t rg = vpadd_s32(r2, g2);

  // Add up all blue values for 8 pixels.
  int32x4_t b = vpaddlq_s16(src.val[2]);
  int32x2_t b2 = vpadd_s32(vget_low_s32(b), vget_high_s32(b));

  int32x2_t ba = vpadd_s32(b2, vdup_n_s32(0));

  return vcombine_s32(rg, ba);
}

ALWAYS_INLINE int32x4_t SumRGB(const int32x4x4_t& src) {
  // Add up all red values for 4 pixels.
  int32x2_t r = vmovn_s64(vpaddlq_s32(src.val[0]));

  // Add up all green values for 4 pixels.
  int32x2_t g = vmovn_s64(vpaddlq_s32(src.val[1]));

  int32x2_t rg = vpadd_s32(r, g);

  // Add up all blue values for 4 pixels.
  int32x2_t b = vmovn_s64(vpaddlq_s32(src.val[2]));

  int32x2_t ba = vpadd_s32(b, vdup_n_s32(0));

  return vcombine_s32(rg, ba);
}

ALWAYS_INLINE int32x4_t DotProduct(int32x4_t r,
                                   int32x4_t g,
                                   int32x4_t b,
                                   int32x4_t dir_r,
                                   int32x4_t dir_g,
                                   int32x4_t dir_b) {
  // Multiply and accumulate each 32 bits element.
  int32x4_t dots = vmulq_s32(r, dir_r);
  dots = vmlaq_s32(dots, g, dir_g);
  dots = vmlaq_s32(dots, b, dir_b);
  return dots;
}

ALWAYS_INLINE int32x4x4_t CalculateDots(const int32x4x4_t& r,
                                        const int32x4x4_t& g,
                                        const int32x4x4_t& b,
                                        const int32x4_t& v_vec) {
  // Duplicate the red, green and blue luminance values.
  int32x4_t r_vec = vdupq_n_s32(vgetq_lane_s32(v_vec, 0));
  int32x4_t g_vec = vdupq_n_s32(vgetq_lane_s32(v_vec, 1));
  int32x4_t b_vec = vdupq_n_s32(vgetq_lane_s32(v_vec, 2));

  int32x4x4_t result;
  result.val[0] = DotProduct(r.val[0], g.val[0], b.val[0], r_vec, g_vec, b_vec);
  result.val[1] = DotProduct(r.val[1], g.val[1], b.val[1], r_vec, g_vec, b_vec);
  result.val[2] = DotProduct(r.val[2], g.val[2], b.val[2], r_vec, g_vec, b_vec);
  result.val[3] = DotProduct(r.val[3], g.val[3], b.val[3], r_vec, g_vec, b_vec);
  return result;
}

// Quantize given colors from 888rgb to 565rgb.
// in:  [min_r min_g min_b 0 max_r max_g max_b 0]
// out: [min_r5 min_g6 min_b5 0][max_r5 max_g6 max_b5 0]
ALWAYS_INLINE uint16x8_t QuantizeTo565(uint8x8_t pixels) {
  // Expand the components to signed 16 bit.
  uint16x8_t pixels16 = vmovl_u8(pixels);

  // {31, 63, 31, 0, 31, 63, 31, 0};
  const uint16x8_t kMultiply = vreinterpretq_u16_u64(vdupq_n_u64(0x1f003f001f));
  uint16x8_t pixel0 = vmulq_u16(pixels16, kMultiply);

  // {128, 128, 128, 0, 128, 128, 128, 0};
  const uint16x8_t kAdd = vreinterpretq_u16_u64(vdupq_n_u64(0x8000800080));
  uint16x8_t pixel1 = vaddq_u16(pixel0, kAdd);

  // Create a shifted copy.
  uint16x8_t pixel2 = vsraq_n_u16(pixel1, pixel1, 8);

  // Shift and return.
  return vshrq_n_u16(pixel2, 8);
}

// Combine the components of base colors in to 16 bits.
// in:  [max_r5 max_g6 max_b5 0][min_r5 min_g6 min_b5 0]
// out: [max_rgb565 min_rgb565 0 0]
ALWAYS_INLINE uint16x4_t PackBaseColors(uint16x8_t base_colors) {
  // Shift to pack RGB565 in 16-bit.
  uint64x2_t r =
      vshlq_u64(vreinterpretq_u64_u16(base_colors), vdupq_n_s64(-32));
  uint64x2_t g =
      vshlq_u64(vreinterpretq_u64_u16(base_colors), vdupq_n_s64(-11));
  uint64x2_t b = vshlq_u64(vreinterpretq_u64_u16(base_colors), vdupq_n_s64(11));
  uint64x2_t base_colors_16 = vorrq_u64(r, vorrq_u64(g, b));

  // Shift to pack 16-bit base colors in 32-bit and return.
  return vreinterpret_u16_u64(
      vorr_u64(vshl_n_u64(vget_high_u64(base_colors_16), 16),
               vand_u64(vget_low_u64(base_colors_16), vdup_n_u64(0xffff))));
}

// Combine the given color indices.
//
// Params:
//  S        Size of an index in bits.
//  indices  Indices to be combined. Each of 8 bits element represents an index.
template <int S>
ALWAYS_INLINE uint64x1_t PackIndices(uint8x16_t indices) {
  uint64x2_t ind = vshlq_n_u64(vreinterpretq_u64_u8(indices), 8 - S);
  const uint64x2_t mask = vdupq_n_u64(0xff00000000000000);
  uint64x2_t ind2 = vandq_u64(vshlq_n_u64(ind, 56), mask);
  ind2 = vorrq_u64(vshrq_n_u64(ind2, S), vandq_u64(vshlq_n_u64(ind, 48), mask));
  ind2 = vorrq_u64(vshrq_n_u64(ind2, S), vandq_u64(vshlq_n_u64(ind, 40), mask));
  ind2 = vorrq_u64(vshrq_n_u64(ind2, S), vandq_u64(vshlq_n_u64(ind, 32), mask));
  ind2 = vorrq_u64(vshrq_n_u64(ind2, S), vandq_u64(vshlq_n_u64(ind, 24), mask));
  ind2 = vorrq_u64(vshrq_n_u64(ind2, S), vandq_u64(vshlq_n_u64(ind, 16), mask));
  ind2 = vorrq_u64(vshrq_n_u64(ind2, S), vandq_u64(vshlq_n_u64(ind, 8), mask));
  ind2 = vorrq_u64(vshrq_n_u64(ind2, S), vandq_u64(ind, mask));
  return vshr_n_u64(
      vorr_u64(vshr_n_u64(vget_low_u64(ind2), (8 * S)), vget_high_u64(ind2)),
      64 - 16 * S);
}

ALWAYS_INLINE int32x4_t
CovarianceChannels(const int16x8x2_t& ch1, const int16x8x2_t& ch2) {
  // Multiply and accumulate.
  int32x4_t cov;
  cov = vmull_s16(vget_low_s16(ch1.val[0]), vget_low_s16(ch2.val[0]));
  cov = vmlal_s16(cov, vget_high_s16(ch1.val[0]), vget_high_s16(ch2.val[0]));
  cov = vmlal_s16(cov, vget_low_s16(ch1.val[1]), vget_low_s16(ch2.val[1]));
  cov = vmlal_s16(cov, vget_high_s16(ch1.val[1]), vget_high_s16(ch2.val[1]));
  return cov;
}

ALWAYS_INLINE int32x4x2_t
Covariance(uint16x4_t average_rgb, const uint8x16x4_t& pixels_scattered) {
  int16x8_t average_r = vreinterpretq_s16_u16(vdupq_lane_u16(average_rgb, 0));
  int16x8_t average_g = vreinterpretq_s16_u16(vdupq_lane_u16(average_rgb, 1));
  int16x8_t average_b = vreinterpretq_s16_u16(vdupq_lane_u16(average_rgb, 2));

  // Subtract red values from the average red.
  int16x8x2_t diff_r;
  diff_r.val[0] = vsubq_s16(
      vreinterpretq_s16_u16(vmovl_u8(vget_low_u8(pixels_scattered.val[0]))),
      average_r);
  diff_r.val[1] = vsubq_s16(
      vreinterpretq_s16_u16(vmovl_u8(vget_high_u8(pixels_scattered.val[0]))),
      average_r);

  // Subtract green values from the average green.
  int16x8x2_t diff_g;
  diff_g.val[0] = vsubq_s16(
      vreinterpretq_s16_u16(vmovl_u8(vget_low_u8(pixels_scattered.val[1]))),
      average_g);
  diff_g.val[1] = vsubq_s16(
      vreinterpretq_s16_u16(vmovl_u8(vget_high_u8(pixels_scattered.val[1]))),
      average_g);

  // Subtract blue values from the average blue.
  int16x8x2_t diff_b;
  diff_b.val[0] = vsubq_s16(
      vreinterpretq_s16_u16(vmovl_u8(vget_low_u8(pixels_scattered.val[2]))),
      average_b);
  diff_b.val[1] = vsubq_s16(
      vreinterpretq_s16_u16(vmovl_u8(vget_high_u8(pixels_scattered.val[2]))),
      average_b);

  int32x4x4_t cov1;
  cov1.val[0] = CovarianceChannels(diff_r, diff_r);
  cov1.val[1] = CovarianceChannels(diff_r, diff_g);
  cov1.val[2] = CovarianceChannels(diff_r, diff_b);
  cov1.val[3] = vdupq_n_s32(0);

  int32x4x4_t cov2;
  cov2.val[0] = CovarianceChannels(diff_g, diff_g);
  cov2.val[1] = CovarianceChannels(diff_g, diff_b);
  cov2.val[2] = CovarianceChannels(diff_b, diff_b);
  cov2.val[3] = vdupq_n_s32(0);

  int32x4x2_t covariance;
  covariance.val[0] = SumRGB(cov1);
  covariance.val[1] = SumRGB(cov2);
  return covariance;
}

ALWAYS_INLINE uint32x2_t MaskOutPixel(const uint8x16x4_t& pixels_linear,
                                      const int32x4x4_t& dots,
                                      int32x4_t max_dot_vec) {
  // Mask out any of the 16 pixels where the dot product matches exactly.
  uint32x4x4_t pixels;
  pixels.val[0] = vandq_u32(vceqq_s32(dots.val[0], max_dot_vec),
                            vreinterpretq_u32_u8(pixels_linear.val[0]));

  pixels.val[1] = vandq_u32(vceqq_s32(dots.val[1], max_dot_vec),
                            vreinterpretq_u32_u8(pixels_linear.val[1]));

  pixels.val[2] = vandq_u32(vceqq_s32(dots.val[2], max_dot_vec),
                            vreinterpretq_u32_u8(pixels_linear.val[2]));

  pixels.val[3] = vandq_u32(vceqq_s32(dots.val[3], max_dot_vec),
                            vreinterpretq_u32_u8(pixels_linear.val[3]));

  // Fold it down.
  return Fold<vec_ops::Max>(pixels);
}

ALWAYS_INLINE uint16x8_t GetBaseColors(const uint8x16x4_t& pixels_linear,
                                       const uint8x16x4_t& pixels_scattered,
                                       int32x4_t dir) {
  // Expand all pixels to signed 32-bit integers.
  int32x4x4_t r = ExpandRGBATo32(pixels_scattered.val[0]);
  int32x4x4_t g = ExpandRGBATo32(pixels_scattered.val[1]);
  int32x4x4_t b = ExpandRGBATo32(pixels_scattered.val[2]);

  int32x4x4_t dots = CalculateDots(r, g, b, dir);

  // Mask out the pixel(s) that matches the max dot.
  uint32x2_t max_pixel =
      MaskOutPixel(pixels_linear, dots, FoldDup<vec_ops::Max>(dots));

  // Mask out the pixel(s) that matches the min dot.
  uint32x2_t min_pixel =
      MaskOutPixel(pixels_linear, dots, FoldDup<vec_ops::Min>(dots));

  return QuantizeTo565(
      vreinterpret_u8_u32(vzip_u32(max_pixel, min_pixel).val[0]));
}

// Figure out the two base colors to use from a block of 16 pixels
// by Primary Component Analysis and map along principal axis.
ALWAYS_INLINE uint16x8_t
OptimizeColorsBlock(const uint8x16x4_t& pixels_linear,
                    const uint8x16x4_t& pixels_scattered,
                    uint16x4_t sum_rgb,
                    uint8x8_t min_rgba,
                    uint8x8_t max_rgba) {
  // min_rgba: [min_r min_g min_b min_a x x x x]
  // max_rgba: [max_r max_g max_b max_a x x x x]

  // Determine color distribution. We already have the max and min, now we need
  // the average of the 16 pixels. Divide sum_rgb with rounding.
  uint16x4_t average_rgb = vrshr_n_u16(sum_rgb, 4);

  // Determine covariance matrix.
  int32x4x2_t covariance = Covariance(average_rgb, pixels_scattered);

  // Convert covariance matrix to float, find principal axis via power
  // iteration.
  float32x4x2_t covariance_float;
  const float32x4_t kInv255 = vdupq_n_f32(1.0f / 255.0f);
  covariance_float.val[0] =
      vmulq_f32(vcvtq_f32_s32(covariance.val[0]), kInv255);
  covariance_float.val[1] =
      vmulq_f32(vcvtq_f32_s32(covariance.val[1]), kInv255);

  int16x4_t max_16 = vreinterpret_s16_u16(vget_low_u16(vmovl_u8(max_rgba)));
  int16x4_t min_16 = vreinterpret_s16_u16(vget_low_u16(vmovl_u8(min_rgba)));
  float32x4_t vf4 = vcvtq_f32_s32(vsubl_s16(max_16, min_16));

  for (int i = 0; i < 4; ++i) {
    float32x4_t vfr4 = vdupq_n_f32(vgetq_lane_f32(vf4, 0));
    float32x4_t vfg4 = vdupq_n_f32(vgetq_lane_f32(vf4, 1));
    float32x4_t vfb4 = vdupq_n_f32(vgetq_lane_f32(vf4, 2));

    // from: [0 1 2 x] [3 4 5 x]
    // to:   [1 3 4 x]
    float32x4_t cov_134 =
        vextq_f32(covariance_float.val[1], covariance_float.val[1], 3);
    cov_134 =
        vsetq_lane_f32(vgetq_lane_f32(covariance_float.val[0], 1), cov_134, 0);

    // from: [0 1 2 x] [3 4 5 x]
    // to:   [2 4 5 x]
    float32x4_t cov_245 = vsetq_lane_f32(
        vgetq_lane_f32(covariance_float.val[0], 2), covariance_float.val[1], 0);

    vf4 = vmulq_f32(vfr4, covariance_float.val[0]);
    vf4 = vmlaq_f32(vf4, vfg4, cov_134);
    vf4 = vmlaq_f32(vf4, vfb4, cov_245);
  }

  float32x4_t magnitude = vabsq_f32(vf4);
  magnitude = vsetq_lane_f32(0.0f, magnitude, 3);  // Null out alpha.
  float32x4_t mag4 = vdupq_lane_f32(
      vpmax_f32(vpmax_f32(vget_low_f32(magnitude), vget_high_f32(magnitude)),
                vdup_n_f32(0.0f)),
      0);

  const int32x4_t kLuminance = {299, 587, 114, 0};

  // Note that this quite often means dividing by zero. The math still works
  // when comparing with Inf though.
  float32x4_t inv_magnitude = Divide<2>(vdupq_n_f32(512.0f), mag4);

  int32x4_t vf4_mag = vcvtq_s32_f32(vmulq_f32(vf4, inv_magnitude));
  int32x4_t v =
      vbslq_s32(vcltq_f32(mag4, vdupq_n_f32(4.0f)), kLuminance, vf4_mag);

  return GetBaseColors(pixels_linear, pixels_scattered, v);
}

ALWAYS_INLINE uint16x8_t
GetApproximateBaseColors(const uint8x16x4_t& pixels_linear,
                         const uint8x16x4_t& pixels_scattered,
                         uint8x8_t min_rgba,
                         uint8x8_t max_rgba) {
  // min_rgba: [min_r min_g min_b min_a x x x x]
  // max_rgba: [max_r max_g max_b max_a x x x x]

  // Get direction vector and expand to 32-bit.
  int16x4_t max_16 = vreinterpret_s16_u16(vget_low_u16(vmovl_u8(max_rgba)));
  int16x4_t min_16 = vreinterpret_s16_u16(vget_low_u16(vmovl_u8(min_rgba)));
  int32x4_t v = vsubl_s16(max_16, min_16);

  return GetBaseColors(pixels_linear, pixels_scattered, v);
}

// Take two base colors and generate 4 RGBX colors where:
//  0 = baseColor0
//  1 = baseColor1
//  2 = (2 * baseColor0 + baseColor1) / 3
//  3 = (2 * baseColor1 + baseColor0) / 3
ALWAYS_INLINE uint16x4x4_t EvalColors(const uint16x8_t& base_colors) {
  // The base colors are expanded by reusing the top bits at the end. That makes
  // sure that white is still white after being quantized and converted back.
  //
  // [(r<<3 | r>>2) (g<<2 | g>>4) (b<<3 | b>>2) 0]

  // The upper shift values for each component.
  // {3, 2, 3, 0, 3, 2, 3, 0};
  const int16x8_t kShiftUp = vreinterpretq_s16_u64(vdupq_n_u64(0x300020003));
  uint16x8_t pixels_up = vshlq_u16(base_colors, kShiftUp);
  // [r0<<3 g0<<2 b0<<3 0] [r1<<3 g1<<2 b1<<3 0]

  // The lower shift values for each component.
  // Note that we need to use negative values to shift right.
  // {-2, -4, -2, 0, -2, -4, -2, 0};
  const int16x8_t kShiftDown =
      vreinterpretq_s16_u64(vdupq_n_u64(0xfffefffcfffe));
  uint16x8_t pixels_down = vshlq_u16(base_colors, kShiftDown);
  // [r0>>2 g0>>4 b0>>2 0] [r1>>2 g1>>4 b1>>2 0]

  uint16x8_t pixels = vorrq_u16(pixels_up, pixels_down);
  // [(r0<<3 | r0>>2) (g0<<2 | g0>>4) (b0<<3 | b0>>2) 0]
  // [(r1<<3 | r1>>2) (g1<<2 | g1>>4) (b1<<3 | b1>>2) 0]

  // Linear interpolate the two other colors:
  // (2 * max + min) / 3
  // (2 * min + max) / 3

  uint16x8_t pixels_mul2 = vaddq_u16(pixels, pixels);

  uint16x8_t swapped = vreinterpretq_u16_u64(vextq_u64(
      vreinterpretq_u64_u16(pixels), vreinterpretq_u64_u16(pixels), 1));
  int16x8_t output = vreinterpretq_s16_u16(vaddq_u16(pixels_mul2, swapped));

  // There's no division in NEON, but we can use "x * ((1 << 16) / 3 + 1))"
  // instead.
  output = vqdmulhq_s16(output, vdupq_n_s16(((1 << 16) / 3 + 1) >> 1));

  uint16x4x4_t colors;
  colors.val[0] = vget_low_u16(pixels);
  colors.val[1] = vget_high_u16(pixels);
  colors.val[2] = vreinterpret_u16_s16(vget_low_s16(output));
  colors.val[3] = vreinterpret_u16_s16(vget_high_s16(output));
  return colors;
}

ALWAYS_INLINE uint8x8_t GetRemapIndices(int32x4_t dots,
                                        int32x4_t half_point,
                                        int32x4_t c0_point,
                                        int32x4_t c3_point) {
  // bits = (dot < half_point ? 4 : 0)
  //      | (dot < c0_point   ? 2 : 0)
  //      | (dot < c3_point   ? 1 : 0)
  int32x4_t cmp0 = vreinterpretq_s32_u32(
      vandq_u32(vcgtq_s32(half_point, dots), vdupq_n_u32(4)));
  int32x4_t cmp1 = vreinterpretq_s32_u32(
      vandq_u32(vcgtq_s32(c0_point, dots), vdupq_n_u32(2)));
  int32x4_t cmp2 = vreinterpretq_s32_u32(
      vandq_u32(vcgtq_s32(c3_point, dots), vdupq_n_u32(1)));
  int32x4_t bits = vorrq_s32(vorrq_s32(cmp0, cmp1), cmp2);

  // Narrow it down to unsigned 8 bits and return.
  return vqmovn_u16(vcombine_u16(vqmovun_s32(bits), vdup_n_u16(0)));
}

// dots:   Dot products for each pixel.
// points: Crossover points.
template <typename T>
ALWAYS_INLINE uint8x16_t GetColorIndices(int32x4x4_t dots, int32x4_t points) {
  // Crossover points for "best color in top half"/"best in bottom half" and
  // the same inside that subinterval.
  int32x4_t c0_point = vdupq_lane_s32(vget_low_s32(points), 1);
  int32x4_t half_point = vdupq_lane_s32(vget_low_s32(points), 0);
  int32x4_t c3_point = vdupq_lane_s32(vget_high_s32(points), 0);

  // Get kRemap table indices.
  uint8x8x4_t ind;
  ind.val[0] = GetRemapIndices(dots.val[0], half_point, c0_point, c3_point);
  ind.val[1] = GetRemapIndices(dots.val[1], half_point, c0_point, c3_point);
  ind.val[2] = GetRemapIndices(dots.val[2], half_point, c0_point, c3_point);
  ind.val[3] = GetRemapIndices(dots.val[3], half_point, c0_point, c3_point);

  // Combine indices.
  uint8x8_t indices_lo =
      vreinterpret_u8_u32(vzip_u32(vreinterpret_u32_u8(ind.val[0]),
                                   vreinterpret_u32_u8(ind.val[1])).val[0]);
  uint8x8_t indices_hi =
      vreinterpret_u8_u32(vzip_u32(vreinterpret_u32_u8(ind.val[2]),
                                   vreinterpret_u32_u8(ind.val[3])).val[0]);
  // Do table lookup and return 2-bit color indices.
  return vcombine_u8(vtbl1_u8(T::kRemap, indices_lo),
                     vtbl1_u8(T::kRemap, indices_hi));
}

template <typename T>
ALWAYS_INLINE uint8x16_t
MatchColorsBlock(const uint8x16x4_t& pixels_scattered, uint16x4x4_t colors) {
  // Get direction vector and expand to 32-bit.
  int32x4_t dir = vsubl_s16(vreinterpret_s16_u16(colors.val[0]),
                            vreinterpret_s16_u16(colors.val[1]));
  // Duplicate r g b elements of direction into different registers.
  int32x4_t dir_r = vdupq_lane_s32(vget_low_s32(dir), 0);
  int32x4_t dir_g = vdupq_lane_s32(vget_low_s32(dir), 1);
  int32x4_t dir_b = vdupq_lane_s32(vget_high_s32(dir), 0);

  // Transpose to separate red, green, blue and alpha channels into 4 different
  // registers. Alpha is ignored.
  uint16x4x2_t trn_lo = vtrn_u16(colors.val[0], colors.val[1]);
  uint16x4x2_t trn_hi = vtrn_u16(colors.val[2], colors.val[3]);
  uint32x4x2_t transposed_colors = vtrnq_u32(
      vreinterpretq_u32_u16(vcombine_u16(trn_lo.val[0], trn_lo.val[1])),
      vreinterpretq_u32_u16(vcombine_u16(trn_hi.val[0], trn_hi.val[1])));

  // Expand to 32-bit.
  int32x4_t colors_r =
      vmovl_s16(vget_low_s16(vreinterpretq_s16_u32(transposed_colors.val[0])));
  int32x4_t colors_g =
      vmovl_s16(vget_high_s16(vreinterpretq_s16_u32(transposed_colors.val[0])));
  int32x4_t colors_b =
      vmovl_s16(vget_low_s16(vreinterpretq_s16_u32(transposed_colors.val[1])));

  // Get dot products.
  int32x4_t stops =
      DotProduct(colors_r, colors_g, colors_b, dir_r, dir_g, dir_b);

  // Build a register containing 4th, 2nd and 3rd elements of stops respectively
  // in each 32 bits element.
  int32x4_t points1 = vsetq_lane_s32(vgetq_lane_s32(stops, 3), stops, 0);
  // Build a register containing 3rd, 4th and 1st elements of stops respectively
  // in each 32 bits element.
  int32x4_t points2 = vreinterpretq_s32_s64(
      vextq_s64(vreinterpretq_s64_s32(stops), vreinterpretq_s64_s32(stops), 1));
  // Add and divide by 2.
  int32x4_t points = vshrq_n_s32(vaddq_s32(points1, points2), 1);

  // Expand all pixels to signed 32-bit integers.
  int32x4x4_t r = ExpandRGBATo32(pixels_scattered.val[0]);
  int32x4x4_t g = ExpandRGBATo32(pixels_scattered.val[1]);
  int32x4x4_t b = ExpandRGBATo32(pixels_scattered.val[2]);

  int32x4x4_t dots = CalculateDots(r, g, b, dir);

  // Get 2-bit color indices.
  return GetColorIndices<T>(dots, points);
}

template <typename T>
ALWAYS_INLINE uint32x4x2_t DoProdsTableLookup(uint8x8_t indices) {
  // Do table lookup for each color index. The values in the table are 3 bytes
  // big so we do it in 3 steps.
  uint16x8_t lookup1 = vmovl_u8(vtbl1_u8(vcreate_u8(T::kProds[0]), indices));
  uint16x8_t lookup2 = vmovl_u8(vtbl1_u8(vcreate_u8(T::kProds[1]), indices));
  uint16x8_t lookup3 = vmovl_u8(vtbl1_u8(vcreate_u8(T::kProds[2]), indices));
  // Expand to 32-bit.
  uint32x4_t lookup1_lo = vmovl_u16(vget_low_u16(lookup1));
  uint32x4_t lookup1_hi = vmovl_u16(vget_high_u16(lookup1));
  uint32x4_t lookup2_lo = vmovl_u16(vget_low_u16(lookup2));
  uint32x4_t lookup2_hi = vmovl_u16(vget_high_u16(lookup2));
  uint32x4_t lookup3_lo = vmovl_u16(vget_low_u16(lookup3));
  uint32x4_t lookup3_hi = vmovl_u16(vget_high_u16(lookup3));
  // Combine results by shifting and or-ing to obtain the actual table value.
  uint32x4x2_t result;
  result.val[0] = vorrq_u32(lookup3_lo, vorrq_u32(vshlq_n_u32(lookup2_lo, 8),
                                                  vshlq_n_u32(lookup1_lo, 16)));
  result.val[1] = vorrq_u32(lookup3_hi, vorrq_u32(vshlq_n_u32(lookup2_hi, 8),
                                                  vshlq_n_u32(lookup1_hi, 16)));
  return result;
}

// Tries to optimize colors to suit block contents better.
// Done by solving a least squares system via normal equations+Cramer's rule.
template <typename T>
ALWAYS_INLINE uint16x8_t RefineBlock(const uint8x16x4_t& pixels_scattered,
                                     uint16x4_t sum_rgb,
                                     uint16x8_t base_colors,
                                     uint8x16_t indices) {
  if (ElementsEqual(indices)) {  // Do all pixels have the same index?
    // Yes, linear system would be singular; solve using optimal single-color
    // match on average color.

    // Get the average of the 16 pixels with rounding.
    uint16x4_t average_rgb = vrshr_n_u16(sum_rgb, 4);

    ALIGNAS(8) uint16_t rgb[4];
    vst1_u16(rgb, average_rgb);
    // Look up optimal values instead of trying to calculate.
    uint16_t colors[8] = {kDXTConstantColors55[rgb[0]][0],
                          Match8BitComponentMax<typename T::BASE_TYPE>(rgb[1]),
                          kDXTConstantColors55[rgb[2]][0],
                          0,
                          kDXTConstantColors55[rgb[0]][1],
                          Match8BitComponentMin<typename T::BASE_TYPE>(rgb[1]),
                          kDXTConstantColors55[rgb[2]][1],
                          0};
    return vld1q_u16(colors);
  } else {
    // Expand to 16-bit.
    int16x8x2_t r = ExpandRGBATo16(pixels_scattered.val[0]);
    int16x8x2_t g = ExpandRGBATo16(pixels_scattered.val[1]);
    int16x8x2_t b = ExpandRGBATo16(pixels_scattered.val[2]);

    // Do table lookup for each color index.
    int8x16_t w1 = DoW1TableLookup<T>(indices);
    // Expand to 16-bit.
    int16x8_t w1_lo = vmovl_s8(vget_low_s8(w1));
    int16x8_t w1_hi = vmovl_s8(vget_high_s8(w1));
    // Multiply and accumulate.
    int16x8x4_t at1_rgb;
    at1_rgb.val[0] = vmulq_s16(w1_lo, r.val[0]);
    at1_rgb.val[0] = vmlaq_s16(at1_rgb.val[0], w1_hi, r.val[1]);
    at1_rgb.val[1] = vmulq_s16(w1_lo, g.val[0]);
    at1_rgb.val[1] = vmlaq_s16(at1_rgb.val[1], w1_hi, g.val[1]);
    at1_rgb.val[2] = vmulq_s16(w1_lo, b.val[0]);
    at1_rgb.val[2] = vmlaq_s16(at1_rgb.val[2], w1_hi, b.val[1]);
    // [r][g][b][]
    int32x4_t at1 = SumRGB(at1_rgb);

    // [r][g][b][]
    int32x4_t at2 = vreinterpretq_s32_u32(vmovl_u16(sum_rgb));
    // at2 = 3 * at2 - at1;
    at2 = vsubq_s32(vmulq_s32(at2, vdupq_n_s32(3)), at1);

    // Do table lookup for each color index.
    uint32x4x2_t akku1 = DoProdsTableLookup<T>(vget_low_u8(indices));
    uint32x4x2_t akku2 = DoProdsTableLookup<T>(vget_high_u8(indices));
    uint32x4_t sum_akku = vaddq_u32(
        vaddq_u32(vaddq_u32(akku1.val[0], akku1.val[1]), akku2.val[0]),
        akku2.val[1]);
    // Pairwise add and accumulate.
    uint64x1_t akku = vpaddl_u32(vget_low_u32(sum_akku));
    akku = vpadal_u32(akku, vget_high_u32(sum_akku));

    // Extract solutions and decide solvability.

    // [akku >> 16]x4
    int32x4_t xx =
        vdupq_lane_s32(vreinterpret_s32_u64(vshr_n_u64(akku, 16)), 0);
    // [(akku >> 8) & 0xff]x4
    const uint64x1_t kFF = vdup_n_u64(0xff);
    int32x4_t yy = vdupq_lane_s32(
        vreinterpret_s32_u64(vand_u64(vshr_n_u64(akku, 8), kFF)), 0);
    // [akku & 0xff]x4
    int32x4_t xy = vdupq_lane_s32(vreinterpret_s32_u64(vand_u64(akku, kFF)), 0);

    // ((3.0f * 31.0f) / 255.0f) / (xx * yy - xy * xy)
    float32x4_t frb = Divide<2>(
        vdupq_n_f32((3.0f * 31.0f) / 255.0f),
        vcvtq_f32_s32(vsubq_s32(vmulq_s32(xx, yy), vmulq_s32(xy, xy))));
    // frb * 63.0f / 31.0f
    float32x4_t fg = vmulq_f32(vmulq_f32(frb, vdupq_n_f32(63.0f)),
                               vdupq_n_f32(1.0f / 31.0f));

    // Solve.

    // [frb][fg][frb][]
    float32x4_t frb_fg_frb = vsetq_lane_f32(vgetq_lane_f32(fg, 0), frb, 1);
    // [31][63][31][]
    const int32x4_t kClamp565_vec = {31, 63, 31, 0};

    // (at1_r * yy - at2_r * xy) * frb + 0.5f
    int32x4_t base0_rgb32 = vcvtq_s32_f32(vaddq_f32(
        vmulq_f32(
            vcvtq_f32_s32(vsubq_s32(vmulq_s32(at1, yy), vmulq_s32(at2, xy))),
            frb_fg_frb),
        vdupq_n_f32(0.5f)));
    // Clamp and saturate.
    uint16x4_t base0_rgb16 = vqmovun_s32(vbslq_s32(
        vcgeq_s32(base0_rgb32, kClamp565_vec), kClamp565_vec, base0_rgb32));

    // (at2_r * xx - at1_r * xy) * frb + 0.5f
    int32x4_t base1_rgb32 = vcvtq_s32_f32(vaddq_f32(
        vmulq_f32(
            vcvtq_f32_s32(vsubq_s32(vmulq_s32(at2, xx), vmulq_s32(at1, xy))),
            frb_fg_frb),
        vdupq_n_f32(0.5f)));
    // Clamp and saturate.
    uint16x4_t base1_rgb16 = vqmovun_s32(vbslq_s32(
        vcgeq_s32(base1_rgb32, kClamp565_vec), kClamp565_vec, base1_rgb32));

    return vcombine_u16(base0_rgb16, base1_rgb16);
  }
}

template <typename T>
ALWAYS_INLINE void CompressColorBlock(uint8_t* dst,
                                      const uint8x16x4_t& pixels_linear,
                                      const uint8x16x4_t& pixels_scattered,
                                      uint8x8_t min_rgba,
                                      uint8x8_t max_rgba,
                                      TextureCompressor::Quality quality) {
  // Take a shortcut if the block is constant (disregarding alpha).
  uint32_t min32 = vget_lane_u32(vreinterpret_u32_u8(min_rgba), 0);
  uint32_t max32 = vget_lane_u32(vreinterpret_u32_u8(max_rgba), 0);
  if ((min32 & 0x00ffffff) == (max32 & 0x00ffffff)) {
    int r = min32 & 0xff;
    int g = (min32 >> 8) & 0xff;
    int b = (min32 >> 16) & 0xff;

    uint16_t max16 = Match8BitColorMax<typename T::BASE_TYPE>(r, g, b);
    uint16_t min16 = Match8BitColorMin<typename T::BASE_TYPE>(r, g, b);
    uint32_t indices = T::kConstantColorIndices;
    FormatFixup<typename T::BASE_TYPE>(&max16, &min16, &indices);

    uint32_t* dst32 = reinterpret_cast<uint32_t*>(dst);
    dst32[0] = max16 | (min16 << 16);
    dst32[1] = indices;
  } else {
    uint16x4_t sum_rgb = vdup_n_u16(0);
    uint16x8_t base_colors;

    if (quality == TextureCompressor::kQualityLow) {
      base_colors = GetApproximateBaseColors(pixels_linear, pixels_scattered,
                                             min_rgba, max_rgba);
    } else {
      sum_rgb = SumRGB(pixels_scattered);
      // Do Primary Component Analysis and map along principal axis.
      base_colors = OptimizeColorsBlock(pixels_linear, pixels_scattered,
                                        sum_rgb, min_rgba, max_rgba);
    }

    // Check if the two base colors are the same.
    uint8x16_t indices;
    if (!Equal(vget_low_u16(base_colors), vget_high_u16(base_colors))) {
      // Calculate the two intermediate colors as well.
      uint16x4x4_t colors = EvalColors(base_colors);

      // Do a first pass to find good index candicates for all 16 of the pixels
      // in the block.
      indices = MatchColorsBlock<T>(pixels_scattered, colors);
    } else {
      // Any indices can be used here.
      indices = vdupq_n_u8(0);
    }

    if (quality == TextureCompressor::kQualityHigh) {
      // Refine the base colors and indices multiple times if requested.
      for (int i = 0; i < kNumRefinements; ++i) {
        uint8x16_t last_indices = indices;
        uint16x8_t last_base_colors = base_colors;

        base_colors =
            RefineBlock<T>(pixels_scattered, sum_rgb, base_colors, indices);
        if (!Equal(last_base_colors, base_colors)) {
          if (!Equal(vget_low_u16(base_colors), vget_high_u16(base_colors))) {
            uint16x4x4_t colors = EvalColors(base_colors);
            indices = MatchColorsBlock<T>(pixels_scattered, colors);
          } else {
            // We ended up with two identical base colors, can't refine this
            // further.
            indices = vdupq_n_u8(0);
            break;
          }
        }

        if (Equal(indices, last_indices)) {
          // There's no need to do another refinement pass if we didn't get any
          // improvements this pass.
          break;
        }
      }
    }

    // Prepare the final block by converting the base colors to 16-bit and
    // packing the pixel indices.
    uint16x4_t base_colors_16 = PackBaseColors(base_colors);
    uint64x1_t indices_2x16 = PackIndices<2>(indices);
    FormatFixupIdx<T>(&base_colors_16, &indices_2x16);
    uint64x1_t output = vorr_u64(vshl_n_u64(indices_2x16, 32),
                                 vreinterpret_u64_u16(base_colors_16));
    vst1_u64(reinterpret_cast<uint64_t*>(dst), output);
  }
}

// alpha: 8x8-bit alpha values.
// dist:  Distance between max and min alpha in the color block.
// bias:  Rounding bias.
ALWAYS_INLINE uint8x8_t
GetAlphaIndices(uint8x8_t alpha, int16x8_t dist, int16x8_t bias) {
  // Expand to signed 16-bit.
  int16x8_t alpha_16 = vreinterpretq_s16_u16(vmovl_u8(alpha));

  // Multiply each alpha value by 7 and add bias.
  int16x8_t a = vaddq_s16(vmulq_s16(alpha_16, vdupq_n_s16(7)), bias);

  int16x8_t dist4 = vmulq_s16(dist, vdupq_n_s16(4));
  int16x8_t dist2 = vmulq_s16(dist, vdupq_n_s16(2));

  // Select index. This is a "linear scale" lerp factor between 0 (val=min)
  // and 7 (val=max).
  // t = (a >= dist4) ? -1 : 0
  int16x8_t t =
      vandq_s16(vreinterpretq_s16_u16(vcgeq_s16(a, dist4)), vdupq_n_s16(-1));
  // ind1 = t & 4;
  int16x8_t ind1 = vandq_s16(t, vdupq_n_s16(4));
  // a1 = a - (dist4 & t);
  int16x8_t a1 = vsubq_s16(a, vandq_s16(dist4, t));

  // t = (a1 >= dist2) ? -1 : 0;
  t = vandq_s16(vreinterpretq_s16_u16(vcgeq_s16(a1, dist2)), vdupq_n_s16(-1));
  // ind2 = t & 2;
  int16x8_t ind2 = vandq_s16(t, vdupq_n_s16(2));
  // a2 = a1 - (dist2 & t);
  int16x8_t a2 = vsubq_s16(a1, vandq_s16(dist2, t));

  // ind3 = (a2 >= dist)
  int16x8_t ind3 =
      vandq_s16(vreinterpretq_s16_u16(vcgeq_s16(a2, dist)), vdupq_n_s16(1));

  // indices = ind1 + ind2 + ind3
  int16x8_t indices = vaddq_s16(ind1, vaddq_s16(ind2, ind3));

  // Turn linear scale into alpha index (0/1 are extremal pts).
  // ind = -indices & 7
  int16x8_t ind = vandq_s16(vnegq_s16(indices), vdupq_n_s16(7));
  // indices = ind ^ (2 > ind)
  indices = veorq_s16(
      ind, vandq_s16(vreinterpretq_s16_u16(vcgtq_s16(vdupq_n_s16(2), ind)),
                     vdupq_n_s16(1)));
  // Narrow it down to unsigned 8 bits and return.
  return vqmovun_s16(indices);
}

ALWAYS_INLINE void CompressAlphaBlock(uint8_t* dst,
                                      uint8x16_t pixels_alpha,
                                      uint8x8_t min_rgba,
                                      uint8x8_t max_rgba) {
  // Take a shortcut if the block is constant.
  uint8_t min_alpha = vget_lane_u8(min_rgba, 3);
  uint8_t max_alpha = vget_lane_u8(max_rgba, 3);
  if (min_alpha == max_alpha) {
    dst[0] = max_alpha;
    dst[1] = min_alpha;
    // All indices are the same, any value will do.
    *reinterpret_cast<uint16_t*>(dst + 2) = 0;
    *reinterpret_cast<uint32_t*>(dst + 4) = 0;
  } else {
    // [max - min]x8
    int16x8_t dist = vdupq_lane_s16(
        vreinterpret_s16_u16(vget_low_u16(vsubl_u8(max_rgba, min_rgba))), 3);
    // bias = (dist < 8) ? (dist - 1) : (dist / 2 + 2)
    int16x8_t bias = vbslq_s16(vcltq_s16(dist, vdupq_n_s16(8)),
                               vsubq_s16(dist, vdupq_n_s16(1)),
                               vaddq_s16(vshrq_n_s16(dist, 1), vdupq_n_s16(2)));
    // bias -= min * 7;
    bias = vsubq_s16(
        bias,
        vmulq_s16(
            vdupq_lane_s16(
                vreinterpret_s16_u16(vget_low_u16(vmovl_u8(min_rgba))), 3),
            vdupq_n_s16(7)));

    uint8x8_t indices_lo =
        GetAlphaIndices(vget_low_u8(pixels_alpha), dist, bias);
    uint8x8_t indices_hi =
        GetAlphaIndices(vget_high_u8(pixels_alpha), dist, bias);

    // Prepare the final block by combining the base alpha values and packing
    // the alpha indices.
    uint8x8_t max_min_alpha = vzip_u8(max_rgba, min_rgba).val[0];
    uint64x1_t indices = PackIndices<3>(vcombine_u8(indices_lo, indices_hi));
    uint64x1_t output =
        vorr_u64(vshl_n_u64(indices, 16),
                 vshr_n_u64(vreinterpret_u64_u8(max_min_alpha), 48));
    vst1_u64(reinterpret_cast<uint64_t*>(dst), output);
  }
}

void ExtractMisalignedBlock(uint8_t* dst,
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

void CompressATC_NEON(const uint8_t* src,
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
    // Number of rows to read in this iteration.
    int num_rows = std::min(height - y, 4);

    for (int x = 0; x < width; x += 4) {
      // Number of columns to read in this iteration.
      int num_columns = std::min(width - x, 4);

      // Load the four rows of pixels.
      uint8x16x4_t pixels_linear;
      if (num_rows + num_columns != 8) {
        // We're at the border and don't have a full block to work with. Extend
        // the last column and row to fill out the block.
        uint64_t aligned_buffer[8];
        uint8_t* buffer = reinterpret_cast<uint8_t*>(aligned_buffer);
        ExtractMisalignedBlock(buffer, src + x * 4, num_columns, num_rows,
                               width);
        pixels_linear.val[0] = vld1q_u8(buffer + 0 * 16);
        pixels_linear.val[1] = vld1q_u8(buffer + 1 * 16);
        pixels_linear.val[2] = vld1q_u8(buffer + 2 * 16);
        pixels_linear.val[3] = vld1q_u8(buffer + 3 * 16);
      } else {
        pixels_linear.val[0] = vld1q_u8(src + (x + 0 * width) * 4);
        pixels_linear.val[1] = vld1q_u8(src + (x + 1 * width) * 4);
        pixels_linear.val[2] = vld1q_u8(src + (x + 2 * width) * 4);
        pixels_linear.val[3] = vld1q_u8(src + (x + 3 * width) * 4);
      }

      // Transpose/scatter the red, green, blue and alpha channels into
      // separate registers.
      ALIGNAS(8) uint8_t block[64];
      vst1q_u8(block + 0 * 16, pixels_linear.val[0]);
      vst1q_u8(block + 1 * 16, pixels_linear.val[1]);
      vst1q_u8(block + 2 * 16, pixels_linear.val[2]);
      vst1q_u8(block + 3 * 16, pixels_linear.val[3]);
      uint8x16x4_t pixels_scattered = vld4q_u8(block);

      // We need the min and max values both to detect solid blocks and when
      // computing the base colors.
      uint8x8_t min_rgba = FoldRGBA<vec_ops::Min>(pixels_scattered);
      uint8x8_t max_rgba = FoldRGBA<vec_ops::Max>(pixels_scattered);

      if (!opaque) {
        CompressAlphaBlock(dst, pixels_scattered.val[3], min_rgba, max_rgba);
        dst += 8;
      }

      CompressColorBlock<TYPE_ATC_NEON>(dst, pixels_linear, pixels_scattered,
                                        min_rgba, max_rgba, quality);
      dst += 8;
    }
  }
}

void CompressDXT_NEON(const uint8_t* src,
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
    // Number of rows to read in this iteration.
    int num_rows = std::min(height - y, 4);

    for (int x = 0; x < width; x += 4) {
      // Number of columns to read in this iteration.
      int num_columns = std::min(width - x, 4);

      // Load the four rows of pixels.
      uint8x16x4_t pixels_linear;
      if (num_rows + num_columns != 8) {
        // We're at the border and don't have a full block to work with. Extend
        // the last column and row to fill out the block.
        uint64_t aligned_buffer[8];
        uint8_t* buffer = reinterpret_cast<uint8_t*>(aligned_buffer);
        ExtractMisalignedBlock(buffer, src + x * 4, num_columns, num_rows,
                               width);
        pixels_linear.val[0] = vld1q_u8(buffer + 0 * 16);
        pixels_linear.val[1] = vld1q_u8(buffer + 1 * 16);
        pixels_linear.val[2] = vld1q_u8(buffer + 2 * 16);
        pixels_linear.val[3] = vld1q_u8(buffer + 3 * 16);
      } else {
        pixels_linear.val[0] = vld1q_u8(src + (x + 0 * width) * 4);
        pixels_linear.val[1] = vld1q_u8(src + (x + 1 * width) * 4);
        pixels_linear.val[2] = vld1q_u8(src + (x + 2 * width) * 4);
        pixels_linear.val[3] = vld1q_u8(src + (x + 3 * width) * 4);
      }

      // Transpose/scatter the red, green, blue and alpha channels into
      // separate registers.
      ALIGNAS(8) uint8_t block[64];
      vst1q_u8(block + 0 * 16, pixels_linear.val[0]);
      vst1q_u8(block + 1 * 16, pixels_linear.val[1]);
      vst1q_u8(block + 2 * 16, pixels_linear.val[2]);
      vst1q_u8(block + 3 * 16, pixels_linear.val[3]);
      uint8x16x4_t pixels_scattered = vld4q_u8(block);

      // We need the min and max values both to detect solid blocks and when
      // computing the base colors.
      uint8x8_t min_rgba = FoldRGBA<vec_ops::Min>(pixels_scattered);
      uint8x8_t max_rgba = FoldRGBA<vec_ops::Max>(pixels_scattered);

      if (!opaque) {
        CompressAlphaBlock(dst, pixels_scattered.val[3], min_rgba, max_rgba);
        dst += 8;
      }

      CompressColorBlock<TYPE_DXT_NEON>(dst, pixels_linear, pixels_scattered,
                                        min_rgba, max_rgba, quality);
      dst += 8;
    }
  }
}
