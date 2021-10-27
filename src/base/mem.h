#ifndef BASE_MEM_H
#define BASE_MEM_H

#include <cstdlib>
#include <memory>

#if defined(__ANDROID__)
#include <malloc.h>
#endif

#include "base/log.h"

#define ALIGN_MEM(alignment) __attribute__((aligned(alignment)))

namespace base {

namespace internal {

struct ScopedAlignedFree {
  inline void operator()(void* x) const {
    if (x)
      free(x);
  }
};

}  // namespace internal

template <typename T>
using AlignedMemPtr = std::unique_ptr<T, internal::ScopedAlignedFree>;

template <int kAlignment>
inline void* AlignedAlloc(size_t size) {
  void* ptr = NULL;
#if defined(__ANDROID__)
  ptr = memalign(kAlignment, size);
#else
  if (posix_memalign(&ptr, kAlignment, size))
    ptr = NULL;
#endif
  DCHECK(ptr);
  // DCHECK(((unsigned)ptr & (kAlignment - 1)) == 0);
  return ptr;
}

inline void AlignedFree(void* mem) {
  free(mem);
}

template <int kAlignment>
inline bool IsAligned(void* ptr) {
  return (reinterpret_cast<uintptr_t>(ptr) & (kAlignment - 1)) == 0U;
}

}  // namespace base

#endif  // BASE_MEM_H
