#include "base/mem.h"

// Use aligned memory for SIMD in texture compressor.
#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_STDIO
#define STBI_MALLOC(sz) base::AlignedAlloc(sz, 16)
#define STBI_REALLOC_SIZED(p, oldsz, newsz) \
  base::AlignedRealloc(p, oldsz, newsz, 16)
#define STBI_FREE(p) base::AlignedFree(p)
#include "third_party/stb/stb_image.h"
