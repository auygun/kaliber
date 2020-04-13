// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TEXTURE_COMPRESSOR_ETC1_NEON_H_
#define TEXTURE_COMPRESSOR_ETC1_NEON_H_

#include <stdint.h>

#include "texture_compressor.h"

class TextureCompressorETC1_NEON : public TextureCompressor {
 public:
  TextureCompressorETC1_NEON() {
    format_ = kFormatETC1;
  }

  // Compress a texture using ETC1. Note that the |quality| parameter is
  // ignored. The current implementation does not support different quality
  // settings.
  void Compress(const uint8_t* src,
                uint8_t* dst,
                int width,
                int height,
                Quality quality) override;
};

#endif  // TEXTURE_COMPRESSOR_ETC1_NEON_H_
