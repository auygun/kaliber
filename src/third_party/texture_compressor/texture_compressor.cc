// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "texture_compressor.h"

#include "dxt_encoder.h"
#include "texture_compressor_etc1.h"

#if defined(__ANDROID__)
#if defined(__ARMEL__) || defined(__aarch64__) || defined(_M_ARM64)
#define ANDROID_NEON
#include <cpu-features.h>
#include "dxt_encoder_neon.h"
#include "texture_compressor_etc1_neon.h"
#endif
#endif

class TextureCompressorATC : public TextureCompressor {
 public:
  /**
   * Creates an ATC encoder. It may create either an ATC or ATCIA encoder,
   * depending on whether opacity support is needed.
   */
  explicit TextureCompressorATC(bool supports_opacity)
      : supports_opacity_(supports_opacity) {
    format_ = supports_opacity_ ? kFormatATCIA : kFormatATC;
  }

  void Compress(const uint8_t* src,
                uint8_t* dst,
                int width,
                int height,
                Quality quality) {
      CompressATC(src, dst, width, height, !supports_opacity_, quality);
  }

 private:
  bool supports_opacity_;
};

#ifdef ANDROID_NEON

class TextureCompressorATC_NEON : public TextureCompressor {
 public:
  /**
   * Creates an ATC encoder. It may create either an ATC or ATCIA encoder,
   * depending on whether opacity support is needed.
   */
  explicit TextureCompressorATC_NEON(bool supports_opacity)
      : supports_opacity_(supports_opacity) {
    format_ = supports_opacity_ ? kFormatATCIA : kFormatATC;
  }

  void Compress(const uint8_t* src,
                uint8_t* dst,
                int width,
                int height,
                Quality quality) {
      CompressATC_NEON(src, dst, width, height, !supports_opacity_, quality);
  }

 private:
  bool supports_opacity_;
};

#endif

class TextureCompressorDXT : public TextureCompressor {
 public:
  /**
   * Creates an ATC encoder. It may create either an ATC or ATCIA encoder,
   * depending on whether opacity support is needed.
   */
  explicit TextureCompressorDXT(bool supports_opacity)
      : supports_opacity_(supports_opacity) {
    format_ = supports_opacity_ ? kFormatDXT5 : kFormatDXT1;
  }

  void Compress(const uint8_t* src,
                uint8_t* dst,
                int width,
                int height,
                Quality quality) {
      CompressDXT(src, dst, width, height, !supports_opacity_, quality);
  }

 private:
  bool supports_opacity_;
};

#ifdef ANDROID_NEON

class TextureCompressorDXT_NEON : public TextureCompressor {
 public:
  /**
   * Creates an ATC encoder. It may create either an ATC or ATCIA encoder,
   * depending on whether opacity support is needed.
   */
  explicit TextureCompressorDXT_NEON(bool supports_opacity)
      : supports_opacity_(supports_opacity) {
    format_ = supports_opacity_ ? kFormatDXT5 : kFormatDXT1;
  }

  void Compress(const uint8_t* src,
                uint8_t* dst,
                int width,
                int height,
                Quality quality) {
      CompressDXT_NEON(src, dst, width, height, !supports_opacity_, quality);
  }

 private:
  bool supports_opacity_;
};

#endif

std::unique_ptr<TextureCompressor> TextureCompressor::Create(Format format) {
  switch (format) {
    case kFormatATC:
    case kFormatATCIA:
#ifdef ANDROID_NEON
      if ((android_getCpuFeatures() & ANDROID_CPU_ARM_FEATURE_NEON) != 0) {
        return std::make_unique<TextureCompressorATC_NEON>(
            format == kFormatATCIA);
      }
#endif
      return std::make_unique<TextureCompressorATC>(format == kFormatATCIA);

    case kFormatDXT1:
    case kFormatDXT5:
#ifdef ANDROID_NEON
      if ((android_getCpuFeatures() & ANDROID_CPU_ARM_FEATURE_NEON) != 0) {
        return std::make_unique<TextureCompressorDXT_NEON>(
            format == kFormatDXT5);
      }
#endif
      return std::make_unique<TextureCompressorDXT>(format == kFormatDXT5);

    case kFormatETC1:
#ifdef ANDROID_NEON
      if ((android_getCpuFeatures() & ANDROID_CPU_ARM_FEATURE_NEON) != 0) {
        return std::make_unique<TextureCompressorETC1_NEON>();
      }
#endif
      return std::make_unique<TextureCompressorETC1>();
  }

  return nullptr;
}
