// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// See the following specification for details on the ETC1 format:
// https://www.khronos.org/registry/gles/extensions/OES/OES_compressed_ETC1_RGB8_texture.txt

#include "texture_compressor_etc1_neon.h"

#include <arm_neon.h>

#include <algorithm>
#include <cassert>
#include <limits>

// GCC 4.6 suffers from a bug, raising an internal error when mixing
// interleaved load instructions with linear load instructions. By fiddling
// with variable declaration order this problem can be avoided which is done
// when the following macro is defined.
#if (__GNUC__ == 4) && (__GNUC_MINOR__ == 6)
#define GCC46_INTERNAL_ERROR_WORKAROUND
#endif

#define ALIGNAS(X) __attribute__ ((aligned (X)))

namespace {

template <typename T>
inline T clamp(T val, T min, T max) {
  return val < min ? min : (val > max ? max : val);
}

inline uint8_t round_to_5_bits(float val) {
  return clamp<uint8_t>(val * 31.0f / 255.0f + 0.5f, 0, 31);
}

inline uint8_t round_to_4_bits(float val) {
  return clamp<uint8_t>(val * 15.0f / 255.0f + 0.5f, 0, 15);
}

union Color {
  struct {
    uint8_t b;
    uint8_t g;
    uint8_t r;
    uint8_t a;
  };
  uint8_t components[4];
  uint32_t bits;
};

/*
 * NEON optimized codeword tables.
 *
 * It allows for a single table entry to be loaded into a 64-bit register
 * without duplication and with the alpha channel already cleared.
 *
 * See: Table 3.17.2
 */
ALIGNAS(8) static const int16_t g_codeword_tables_neon_opt[8][16] = {
    {-8, -8, -8, 0, -2, -2, -2, 0, 2, 2, 2, 0, 8, 8, 8, 0},
    {-17, -17, -17, 0, -5, -5, -5, 0, 5, 5, 5, 0, 17, 17, 17, 0},
    {-29, -29, -29, 0, -9, -9, -9, 0, 9, 9, 9, 0, 29, 29, 29, 0},
    {-42, -42, -42, 0, -13, -13, -13, 0, 13, 13, 13, 0, 42, 42, 42, 0},
    {-60, -60, -60, 0, -18, -18, -18, 0, 18, 18, 18, 0, 60, 60, 60, 0},
    {-80, -80, -80, 0, -24, -24, -24, 0, 24, 24, 24, 0, 80, 80, 80, 0},
    {-106, -106, -106, 0, -33, -33, -33, 0, 33, 33, 33, 0, 106, 106, 106, 0},
    {-183, -183, -183, 0, -47, -47, -47, 0, 47, 47, 47, 0, 183, 183, 183, 0}};

/*
 * Maps modifier indices to pixel index values.
 * See: Table 3.17.3
 */
static const uint8_t g_mod_to_pix[4] = {3, 2, 0, 1};

/*
 * The ETC1 specification index texels as follows:
 *
 * [a][e][i][m]     [ 0][ 4][ 8][12]
 * [b][f][j][n] <-> [ 1][ 5][ 9][13]
 * [c][g][k][o]     [ 2][ 6][10][14]
 * [d][h][l][p]     [ 3][ 7][11][15]
 *
 * However, when extracting sub blocks from BGRA data the natural array
 * indexing order ends up different:
 *
 * vertical0: [a][b][c][d]  horizontal0: [a][e][i][m]
 *            [e][f][g][h]               [b][f][j][n]
 * vertical1: [i][j][k][l]  horizontal1: [c][g][k][o]
 *            [m][n][o][p]               [d][h][l][p]
 *
 * In order to translate from the natural array indices in a sub block to the
 * indices (numbers) used by specification and hardware we use this table.
 *
 * NOTE: Since we can efficiently transpose matrixes using NEON we end up with
 *       near perfect indexing for vertical sub blocks.
 */
static const uint8_t g_idx_to_num[4][8] = {
    {0, 1, 2, 3, 4, 5, 6, 7},        // Vertical block 0.
    {8, 9, 10, 11, 12, 13, 14, 15},  // Vertical block 1.
    {0, 4, 8, 12, 1, 5, 9, 13},      // Horizontal block 0.
    {2, 6, 10, 14, 3, 7, 11, 15}     // Horizontal block 1.
};

inline void WriteColors444(uint8_t* block,
                           const Color& color0,
                           const Color& color1) {
  block[0] = (color0.b & 0xf0) | (color1.b >> 4);
  block[1] = (color0.g & 0xf0) | (color1.g >> 4);
  block[2] = (color0.r & 0xf0) | (color1.r >> 4);
}

inline void WriteColors555(uint8_t* block,
                           const Color& color0,
                           const Color& color1) {
  // Table for conversion to 3-bit two complement format.
  static const uint8_t two_compl_trans_table[8] = {
      4,  // -4 (100b)
      5,  // -3 (101b)
      6,  // -2 (110b)
      7,  // -1 (111b)
      0,  //  0 (000b)
      1,  //  1 (001b)
      2,  //  2 (010b)
      3,  //  3 (011b)
  };

  int16_t delta_r = static_cast<int16_t>(color1.r >> 3) - (color0.r >> 3);
  int16_t delta_g = static_cast<int16_t>(color1.g >> 3) - (color0.g >> 3);
  int16_t delta_b = static_cast<int16_t>(color1.b >> 3) - (color0.b >> 3);
  assert(delta_r >= -4 && delta_r <= 3);
  assert(delta_g >= -4 && delta_g <= 3);
  assert(delta_b >= -4 && delta_b <= 3);

  block[0] = (color0.b & 0xf8) | two_compl_trans_table[delta_b + 4];
  block[1] = (color0.g & 0xf8) | two_compl_trans_table[delta_g + 4];
  block[2] = (color0.r & 0xf8) | two_compl_trans_table[delta_r + 4];
}

inline void WriteCodewordTable(uint8_t* block,
                               uint8_t sub_block_id,
                               uint8_t table) {
  uint8_t shift = (2 + (3 - sub_block_id * 3));
  block[3] &= ~(0x07 << shift);
  block[3] |= table << shift;
}

inline void WritePixelData(uint8_t* block, uint32_t pixel_data) {
  block[4] |= pixel_data >> 24;
  block[5] |= (pixel_data >> 16) & 0xff;
  block[6] |= (pixel_data >> 8) & 0xff;
  block[7] |= pixel_data & 0xff;
}

inline void WriteFlip(uint8_t* block, bool flip) {
  block[3] &= ~0x01;
  block[3] |= static_cast<uint8_t>(flip);
}

inline void WriteDiff(uint8_t* block, bool diff) {
  block[3] &= ~0x02;
  block[3] |= static_cast<uint8_t>(diff) << 1;
}

/**
 * Compress and rounds BGR888 into BGR444. The resulting BGR444 color is
 * expanded to BGR888 as it would be in hardware after decompression. The
 * actual 444-bit data is available in the four most significant bits of each
 * channel.
 */
inline Color MakeColor444(const float* bgr) {
  uint8_t b4 = round_to_4_bits(bgr[0]);
  uint8_t g4 = round_to_4_bits(bgr[1]);
  uint8_t r4 = round_to_4_bits(bgr[2]);
  Color bgr444;
  bgr444.b = (b4 << 4) | b4;
  bgr444.g = (g4 << 4) | g4;
  bgr444.r = (r4 << 4) | r4;
  return bgr444;
}

/**
 * Compress and rounds BGR888 into BGR555. The resulting BGR555 color is
 * expanded to BGR888 as it would be in hardware after decompression. The
 * actual 555-bit data is available in the five most significant bits of each
 * channel.
 */
inline Color MakeColor555(const float* bgr) {
  uint8_t b5 = round_to_5_bits(bgr[0]);
  uint8_t g5 = round_to_5_bits(bgr[1]);
  uint8_t r5 = round_to_5_bits(bgr[2]);
  Color bgr555;
  bgr555.b = (b5 << 3) | (b5 >> 2);
  bgr555.g = (g5 << 3) | (g5 >> 2);
  bgr555.r = (r5 << 3) | (r5 >> 2);
  return bgr555;
}

/**
 * Calculates the error metric for two colors. A small error signals that the
 * colors are similar to each other, a large error the signals the opposite.
 *
 * IMPORTANT: This function call has been inlined and NEON optimized in the
 *            ComputeLuminance() function. The inlined version should be kept
 *            in sync with this function implementation.
 */
inline uint32_t GetColorError(const Color& u, const Color& v) {
  int delta_b = static_cast<int>(u.b) - v.b;
  int delta_g = static_cast<int>(u.g) - v.g;
  int delta_r = static_cast<int>(u.r) - v.r;
  return delta_b * delta_b + delta_g * delta_g + delta_r * delta_r;
}

void GetAverageColor(const Color* src, float* avg_color_bgr) {
  const uint8_t* src_ptr = reinterpret_cast<const uint8_t*>(src);
#ifdef GCC46_INTERNAL_ERROR_WORKAROUND
  uint8x8x4_t src0;
  src0 = vld4_u8(src_ptr);
#else
  uint8x8x4_t src0 = vld4_u8(src_ptr);
#endif

  uint64x1_t sum_b0 = vpaddl_u32(vpaddl_u16(vpaddl_u8(src0.val[0])));
  uint64x1_t sum_g0 = vpaddl_u32(vpaddl_u16(vpaddl_u8(src0.val[1])));
  uint64x1_t sum_r0 = vpaddl_u32(vpaddl_u16(vpaddl_u8(src0.val[2])));

  ALIGNAS(8) uint64_t sum_b, sum_g, sum_r;
  vst1_u64(&sum_b, sum_b0);
  vst1_u64(&sum_g, sum_g0);
  vst1_u64(&sum_r, sum_r0);

  const float kInv8 = 1.0f / 8.0f;
  avg_color_bgr[0] = static_cast<float>(sum_b) * kInv8;
  avg_color_bgr[1] = static_cast<float>(sum_g) * kInv8;
  avg_color_bgr[2] = static_cast<float>(sum_r) * kInv8;
}

void ComputeLuminance(uint8_t* block,
                      const Color* src,
                      const Color& base,
                      int sub_block_id,
                      const uint8_t* idx_to_num_tab) {
  uint32_t best_tbl_err = std::numeric_limits<uint32_t>::max();
  uint8_t best_tbl_idx = 0;
  uint8_t best_mod_idxs[8][8];  // [table][texel]

  // Load immutable data that is shared through iteration.
  ALIGNAS(8) const int16_t base_color_ptr[4] = {base.b, base.g, base.r, 0x00};
  int16x8_t base_color =
      vcombine_s16(vld1_s16(base_color_ptr), vld1_s16(base_color_ptr));

  ALIGNAS(8) const uint32_t idx_mask_ptr[4] = {0x00, 0x01, 0x02, 0x03};
  uint32x4_t idx_mask = vld1q_u32(idx_mask_ptr);

  // Preload source color registers.
  uint8x16_t src_color[8];
  for (unsigned int i = 0; i < 8; ++i) {
    const uint32_t* src_ptr = reinterpret_cast<const uint32_t*>(&src[i]);
    src_color[i] = vreinterpretq_u8_u32(vld1q_dup_u32(src_ptr));
  }

  // Try all codeword tables to find the one giving the best results for this
  // block.
  for (unsigned int tbl_idx = 0; tbl_idx < 8; ++tbl_idx) {
    uint32_t tbl_err = 0;

    // For the current table, compute the candidate color: base + lum for all
    // four luminance entries.
    const int16_t* lum_ptr = g_codeword_tables_neon_opt[tbl_idx];
    int16x8_t lum01 = vld1q_s16(lum_ptr);
    int16x8_t lum23 = vld1q_s16(lum_ptr + 8);

    int16x8_t color01 = vaddq_s16(base_color, lum01);
    int16x8_t color23 = vaddq_s16(base_color, lum23);

    // Clamp the candidate colors to [0, 255].
    color01 = vminq_s16(color01, vdupq_n_s16(255));
    color01 = vmaxq_s16(color01, vdupq_n_s16(0));
    color23 = vminq_s16(color23, vdupq_n_s16(255));
    color23 = vmaxq_s16(color23, vdupq_n_s16(0));

    uint8x16_t candidate_color =
        vcombine_u8(vmovn_u16(vreinterpretq_u16_s16(color01)),
                    vmovn_u16(vreinterpretq_u16_s16(color23)));

    for (unsigned int i = 0; i < 8; ++i) {
      // Compute the squared distance between the source and candidate colors.
      uint8x16_t diff = vabdq_u8(src_color[i], candidate_color);
      uint8x8_t diff01 = vget_low_u8(diff);
      uint8x8_t diff23 = vget_high_u8(diff);

      uint16x8_t square01 = vmull_u8(diff01, diff01);
      uint16x8_t square23 = vmull_u8(diff23, diff23);

      uint32x4_t psum01 = vpaddlq_u16(square01);
      uint32x4_t psum23 = vpaddlq_u16(square23);
      uint32x2_t err01 = vpadd_u32(vget_low_u32(psum01), vget_high_u32(psum01));
      uint32x2_t err23 = vpadd_u32(vget_low_u32(psum23), vget_high_u32(psum23));
      uint32x4_t errs = vcombine_u32(err01, err23);

      // Find the minimum error.
      uint32x2_t min_err = vpmin_u32(err01, err23);
      min_err = vpmin_u32(min_err, min_err);

      // Find the modifier index which produced the minimum error. This is
      // essentially the lane number of the lane containing the minimum error.
      uint32x4_t min_mask = vceqq_u32(vcombine_u32(min_err, min_err), errs);
      uint32x4_t idxs = vbslq_u32(min_mask, idx_mask, vdupq_n_u32(0xffffffff));

      uint32x2_t min_idx = vpmin_u32(vget_low_u32(idxs), vget_high_u32(idxs));
      min_idx = vpmin_u32(min_idx, min_idx);

      uint32_t best_mod_err = vget_lane_u32(min_err, 0);
      uint32_t best_mod_idx = vget_lane_u32(min_idx, 0);

      best_mod_idxs[tbl_idx][i] = best_mod_idx;

      tbl_err += best_mod_err;
      if (tbl_err > best_tbl_err)
        break;  // We're already doing worse than the best table so skip.
    }

    if (tbl_err < best_tbl_err) {
      best_tbl_err = tbl_err;
      best_tbl_idx = tbl_idx;

      if (tbl_err == 0)
        break;  // We cannot do any better than this.
    }
  }

  WriteCodewordTable(block, sub_block_id, best_tbl_idx);

  uint32_t pix_data = 0;

  for (unsigned int i = 0; i < 8; ++i) {
    uint8_t mod_idx = best_mod_idxs[best_tbl_idx][i];
    uint8_t pix_idx = g_mod_to_pix[mod_idx];

    uint32_t lsb = pix_idx & 0x1;
    uint32_t msb = pix_idx >> 1;

    // Obtain the texel number as specified in the standard.
    int texel_num = idx_to_num_tab[i];
    pix_data |= msb << (texel_num + 16);
    pix_data |= lsb << (texel_num);
  }

  WritePixelData(block, pix_data);
}

/**
 * Compress a solid, single colored block.
 */
void CompressSolidBlock(uint8_t* dst, Color src) {
  // Clear destination buffer so that we can "or" in the results.
  memset(dst, 0, 8);

  float src_color_float[3] = {static_cast<float>(src.b),
                              static_cast<float>(src.g),
                              static_cast<float>(src.r)};
  Color base = MakeColor555(src_color_float);

  WriteDiff(dst, true);
  WriteFlip(dst, false);
  WriteColors555(dst, base, base);

  uint32_t best_tbl_err = std::numeric_limits<uint32_t>::max();
  uint8_t best_tbl_idx = 0;
  uint8_t best_mod_idx = 0;

  // Load immutable data that is shared through iteration.
  ALIGNAS(8) const int16_t base_color_ptr[4] = {base.b, base.g, base.r, 0x00};
  int16x8_t base_color =
      vcombine_s16(vld1_s16(base_color_ptr), vld1_s16(base_color_ptr));

  ALIGNAS(8) const uint32_t idx_mask_ptr[4] = {0x00, 0x01, 0x02, 0x03};
  uint32x4_t idx_mask = vld1q_u32(idx_mask_ptr);

  // Preload source color registers.
  src.a = 0x00;
  uint8x16_t src_color = vreinterpretq_u8_u32(
      vld1q_dup_u32(reinterpret_cast<const uint32_t*>(&src)));

  // Try all codeword tables to find the one giving the best results for this
  // block.
  for (unsigned int tbl_idx = 0; tbl_idx < 8; ++tbl_idx) {
    // For the current table, compute the candidate color: base + lum for all
    // four luminance entries.
    const int16_t* lum_ptr = g_codeword_tables_neon_opt[tbl_idx];
    int16x8_t lum01 = vld1q_s16(lum_ptr);
    int16x8_t lum23 = vld1q_s16(lum_ptr + 8);

    int16x8_t color01 = vaddq_s16(base_color, lum01);
    int16x8_t color23 = vaddq_s16(base_color, lum23);

    // Clamp the candidate colors to [0, 255].
    color01 = vminq_s16(color01, vdupq_n_s16(255));
    color01 = vmaxq_s16(color01, vdupq_n_s16(0));
    color23 = vminq_s16(color23, vdupq_n_s16(255));
    color23 = vmaxq_s16(color23, vdupq_n_s16(0));

    uint8x16_t candidate_color =
        vcombine_u8(vmovn_u16(vreinterpretq_u16_s16(color01)),
                    vmovn_u16(vreinterpretq_u16_s16(color23)));

    // Compute the squared distance between the source and candidate colors.
    uint8x16_t diff = vabdq_u8(src_color, candidate_color);
    uint8x8_t diff01 = vget_low_u8(diff);
    uint8x8_t diff23 = vget_high_u8(diff);

    uint16x8_t square01 = vmull_u8(diff01, diff01);
    uint16x8_t square23 = vmull_u8(diff23, diff23);

    uint32x4_t psum01 = vpaddlq_u16(square01);
    uint32x4_t psum23 = vpaddlq_u16(square23);
    uint32x2_t err01 = vpadd_u32(vget_low_u32(psum01), vget_high_u32(psum01));
    uint32x2_t err23 = vpadd_u32(vget_low_u32(psum23), vget_high_u32(psum23));
    uint32x4_t errs = vcombine_u32(err01, err23);

    // Find the minimum error.
    uint32x2_t min_err = vpmin_u32(err01, err23);
    min_err = vpmin_u32(min_err, min_err);

    // Find the modifier index which produced the minimum error. This is
    // essentially the lane number of the lane containing the minimum error.
    uint32x4_t min_mask = vceqq_u32(vcombine_u32(min_err, min_err), errs);
    uint32x4_t idxs = vbslq_u32(min_mask, idx_mask, vdupq_n_u32(0xffffffff));

    uint32x2_t min_idx = vpmin_u32(vget_low_u32(idxs), vget_high_u32(idxs));
    min_idx = vpmin_u32(min_idx, min_idx);

    uint32_t cur_best_mod_err = vget_lane_u32(min_err, 0);
    uint32_t cur_best_mod_idx = vget_lane_u32(min_idx, 0);

    uint32_t tbl_err = cur_best_mod_err;
    if (tbl_err < best_tbl_err) {
      best_tbl_err = tbl_err;
      best_tbl_idx = tbl_idx;
      best_mod_idx = cur_best_mod_idx;

      if (tbl_err == 0)
        break;  // We cannot do any better than this.
    }
  }

  WriteCodewordTable(dst, 0, best_tbl_idx);
  WriteCodewordTable(dst, 1, best_tbl_idx);

  uint8_t pix_idx = g_mod_to_pix[best_mod_idx];
  uint32_t lsb = pix_idx & 0x1;
  uint32_t msb = pix_idx >> 1;

  uint32_t pix_data = 0;
  for (unsigned int i = 0; i < 2; ++i) {
    for (unsigned int j = 0; j < 8; ++j) {
      // Obtain the texel number as specified in the standard.
      int texel_num = g_idx_to_num[i][j];
      pix_data |= msb << (texel_num + 16);
      pix_data |= lsb << (texel_num);
    }
  }

  WritePixelData(dst, pix_data);
}

void CompressBlock(uint8_t* dst, const Color* ver_src, const Color* hor_src) {
  ALIGNAS(8) const Color* sub_block_src[4] = {
      ver_src, ver_src + 8, hor_src, hor_src + 8};

  Color sub_block_avg[4];
  bool use_differential[2] = {true, true};

  // Compute the average color for each sub block and determine if differential
  // coding can be used.
  for (unsigned int i = 0, j = 1; i < 4; i += 2, j += 2) {
    float avg_color_0[3];
    GetAverageColor(sub_block_src[i], avg_color_0);
    Color avg_color_555_0 = MakeColor555(avg_color_0);

    float avg_color_1[3];
    GetAverageColor(sub_block_src[j], avg_color_1);
    Color avg_color_555_1 = MakeColor555(avg_color_1);

    for (unsigned int light_idx = 0; light_idx < 3; ++light_idx) {
      int u = avg_color_555_0.components[light_idx] >> 3;
      int v = avg_color_555_1.components[light_idx] >> 3;

      int component_diff = v - u;
      if (component_diff < -4 || component_diff > 3) {
        use_differential[i / 2] = false;
        sub_block_avg[i] = MakeColor444(avg_color_0);
        sub_block_avg[j] = MakeColor444(avg_color_1);
      } else {
        sub_block_avg[i] = avg_color_555_0;
        sub_block_avg[j] = avg_color_555_1;
      }
    }
  }

  // Compute the error of each sub block before adjusting for luminance. These
  // error values are later used for determining if we should flip the sub
  // block or not.
  uint32_t sub_block_err[4] = {0};
  for (unsigned int i = 0; i < 4; ++i) {
    for (unsigned int j = 0; j < 8; ++j) {
      sub_block_err[i] += GetColorError(sub_block_avg[i], sub_block_src[i][j]);
    }
  }

  bool flip =
      sub_block_err[2] + sub_block_err[3] < sub_block_err[0] + sub_block_err[1];

  // Clear destination buffer so that we can "or" in the results.
  memset(dst, 0, 8);

  WriteDiff(dst, use_differential[!!flip]);
  WriteFlip(dst, flip);

  uint8_t sub_block_off_0 = flip ? 2 : 0;
  uint8_t sub_block_off_1 = sub_block_off_0 + 1;

  if (use_differential[!!flip]) {
    WriteColors555(dst, sub_block_avg[sub_block_off_0],
                   sub_block_avg[sub_block_off_1]);
  } else {
    WriteColors444(dst, sub_block_avg[sub_block_off_0],
                   sub_block_avg[sub_block_off_1]);
  }

  // Compute luminance for the first sub block.
  ComputeLuminance(dst, sub_block_src[sub_block_off_0],
                   sub_block_avg[sub_block_off_0], 0,
                   g_idx_to_num[sub_block_off_0]);
  // Compute luminance for the second sub block.
  ComputeLuminance(dst, sub_block_src[sub_block_off_1],
                   sub_block_avg[sub_block_off_1], 1,
                   g_idx_to_num[sub_block_off_1]);
}

// Extract up to a 4x4 block of pixels and "de-swizzle" them into 16x1.
// If 'num_columns' or 'num_rows' are less than 4 then it fills out the rest
// of the block by taking a copy of the last valid column or row.
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

void TextureCompressorETC1_NEON::Compress(const uint8_t* src,
                                          uint8_t* dst,
                                          int width,
                                          int height,
                                          Quality quality) {
  ALIGNAS(8) uint32_t ver_blocks[16];
  ALIGNAS(8) uint32_t hor_blocks[16];

  uint64_t aligned_buffer[8];
  uint8_t* block_buffer = reinterpret_cast<uint8_t*>(aligned_buffer);

  // Mask for clearing the alpha channel.
  ALIGNAS(8) const uint32_t clear_mask_ptr[4] = {
      0xff000000, 0xff000000, 0xff000000, 0xff000000};
  uint32x4_t clear_mask = vld1q_u32(clear_mask_ptr);

  for (int y = 0; y < height; y += 4, src += width * 4 * 4) {
    int num_rows = std::min(height - y, 4);
    for (int x = 0; x < width; x += 4, dst += 8) {
      int num_columns = std::min(width - x, 4);

      const uint32_t* row0;
      int stride;
      if (num_rows + num_columns != 8) {
        ExtractMisalignedBlock(block_buffer, src + x * 4, num_columns, num_rows,
                               width);
        row0 = reinterpret_cast<const uint32_t*>(block_buffer);
        stride = 4;
      } else {
        row0 = reinterpret_cast<const uint32_t*>(src + x * 4);
        stride = width;
      }
      const uint32_t* row1 = row0 + stride;
      const uint32_t* row2 = row1 + stride;
      const uint32_t* row3 = row2 + stride;

#ifdef GCC46_INTERNAL_ERROR_WORKAROUND
      uint32x4x4_t block_transposed;
#endif
      ALIGNAS(8) uint32x4_t block[4];
      block[0] = vld1q_u32(row0);
      block[1] = vld1q_u32(row1);
      block[2] = vld1q_u32(row2);
      block[3] = vld1q_u32(row3);

      // Clear alpha channel.
      for (unsigned int i = 0; i < 4; ++i) {
        block[i] = vbicq_u32(block[i], clear_mask);
      }

      // Check if the block is solid.
      uint32x4_t solid = vbicq_u32(vdupq_n_u32(*row0), clear_mask);

      uint16x4_t eq0 = vmovn_u32(vceqq_u32(block[0], solid));
      uint16x4_t eq1 = vmovn_u32(vceqq_u32(block[1], solid));
      uint16x4_t eq2 = vmovn_u32(vceqq_u32(block[2], solid));
      uint16x4_t eq3 = vmovn_u32(vceqq_u32(block[3], solid));
      uint16x4_t tst = vand_u16(vand_u16(eq0, eq1), vand_u16(eq2, eq3));

      ALIGNAS(8) uint64_t solid_block_tst_bits;
      vst1_u64(&solid_block_tst_bits, vreinterpret_u64_u16(tst));

      if (solid_block_tst_bits == 0xffffffffffffffff) {
        CompressSolidBlock(dst, *reinterpret_cast<const Color*>(row0));
        continue;
      }

      vst1q_u32(hor_blocks, block[0]);
      vst1q_u32(hor_blocks + 4, block[1]);
      vst1q_u32(hor_blocks + 8, block[2]);
      vst1q_u32(hor_blocks + 12, block[3]);

      // Texel ordering according to specification:
      // [ 0][ 4][ 8][12]
      // [ 1][ 5][ 9][13]
      // [ 2][ 6][10][14]
      // [ 3][ 7][11][15]
      //
      // To access the vertical blocks using C-style indexing we
      // transpose the block:
      // [ 0][ 1][ 2][ 3]
      // [ 4][ 5][ 6][ 7]
      // [ 8][ 9][10][11]
      // [12][13][14][15]
#ifdef GCC46_INTERNAL_ERROR_WORKAROUND
      block_transposed = vld4q_u32(hor_blocks);
#else
      uint32x4x4_t block_transposed = vld4q_u32(hor_blocks);
#endif

      vst1q_u32(ver_blocks, block_transposed.val[0]);
      vst1q_u32(ver_blocks + 4, block_transposed.val[1]);
      vst1q_u32(ver_blocks + 8, block_transposed.val[2]);
      vst1q_u32(ver_blocks + 12, block_transposed.val[3]);

      CompressBlock(dst, reinterpret_cast<const Color*>(ver_blocks),
                    reinterpret_cast<const Color*>(hor_blocks));
    }
  }
}
