// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DXT_ENCODER_INTERNALS_H_
#define DXT_ENCODER_INTERNALS_H_

#include <stdint.h>

extern const uint8_t kDXTConstantColors55[256][2];
extern const uint8_t kDXTConstantColors66[256][2];
extern const uint8_t kDXTConstantColors56[256][2];

// Types used to explicitly instantiate template functions.
struct TYPE_ATC {
  static const uint32_t kConstantColorIndices = 0x55555555;
};

struct TYPE_DXT {
  static const uint32_t kConstantColorIndices = 0xaaaaaaaa;
};

// Returns max and min base colors matching the given 8-bit color channels when
// solved via linear interpolation. Output format differs for ATC and DXT. See
// explicitly instantiated template functions below.
template <typename T>
inline uint16_t Match8BitColorMax(int r, int g, int b);
template <typename T>
inline uint16_t Match8BitColorMin(int r, int g, int b);

template <>
inline uint16_t Match8BitColorMax<TYPE_ATC>(int r, int g, int b) {
  return (kDXTConstantColors55[r][0] << 11) |
         (kDXTConstantColors56[g][0] << 6) | kDXTConstantColors55[b][0];
}

template <>
inline uint16_t Match8BitColorMin<TYPE_ATC>(int r, int g, int b) {
  return (kDXTConstantColors55[r][1] << 11) |
         (kDXTConstantColors56[g][1] << 5) | kDXTConstantColors55[b][1];
}

template <>
inline uint16_t Match8BitColorMax<TYPE_DXT>(int r, int g, int b) {
  return (kDXTConstantColors55[r][0] << 11) |
         (kDXTConstantColors66[g][0] << 5) | kDXTConstantColors55[b][0];
}

template <>
inline uint16_t Match8BitColorMin<TYPE_DXT>(int r, int g, int b) {
  return (kDXTConstantColors55[r][1] << 11) |
         (kDXTConstantColors66[g][1] << 5) | kDXTConstantColors55[b][1];
}

// This converts the output data to either ATC or DXT format.
// See explicitly instantiated template functions below.
template <typename T>
inline void FormatFixup(uint16_t* max16, uint16_t* min16, uint32_t* mask);

template <>
inline void FormatFixup<TYPE_ATC>(uint16_t* max16,
                                  uint16_t* min16,
                                  uint32_t* mask) {
  // First color in ATC format is 555.
  *max16 = (*max16 & 0x001f) | ((*max16 & 0xffC0) >> 1);
}

template <>
inline void FormatFixup<TYPE_DXT>(uint16_t* max16,
                                  uint16_t* min16,
                                  uint32_t* mask) {
  // Swap min/max colors if necessary.
  if (*max16 < *min16) {
    uint16_t t = *min16;
    *min16 = *max16;
    *max16 = t;
    *mask ^= 0x55555555;
  }
}

// Number of passes over the block that's done to refine the base colors.
// Only applies to high quality compression mode.
const int kNumRefinements = 2;

#endif  // DXT_ENCODER_INTERNALS_H_
