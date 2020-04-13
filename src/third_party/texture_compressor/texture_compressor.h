// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TEXTURE_COMPRESSOR_H_
#define TEXTURE_COMPRESSOR_H_

#include <stdint.h>
#include <memory>

class TextureCompressor {
 public:
  enum Format {
    kFormatATC,
    kFormatATCIA,
    kFormatDXT1,
    kFormatDXT5,
    kFormatETC1,
  };

  enum Quality {
    kQualityLow,
    kQualityMedium,
    kQualityHigh,
  };

  static std::unique_ptr<TextureCompressor> Create(Format format);
  virtual ~TextureCompressor() {}

  virtual void Compress(const uint8_t* src,
                        uint8_t* dst,
                        int width,
                        int height,
                        Quality quality) = 0;

  Format format() { return format_; }

 protected:
  TextureCompressor() {}

  Format format_;
};

#endif  // TEXTURE_COMPRESSOR_H_
