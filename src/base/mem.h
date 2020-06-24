#ifndef MEM_H
#define MEM_H

#include <cassert>
#include <cstdlib>
#include <memory>

#if defined(__ANDROID__)
#include <malloc.h>
#endif

#define ALIGN_MEM(alignment) __attribute__((aligned(alignment)))

namespace internal {

struct ScopedAlignedFree {
  inline void operator()(void* x) const {
    if (x)
      free(x);
  }
};

}  // namespace internal

namespace base {

template <typename T>
struct AlignedMem {
  using ScoppedPtr = std::unique_ptr<T, internal::ScopedAlignedFree>;
};

template <int kAlignment>
inline void* AlignedAlloc(size_t size) {
  void* ptr = NULL;
#if defined(__ANDROID__)
  ptr = memalign(kAlignment, size);
#else
  if (posix_memalign(&ptr, kAlignment, size))
    ptr = NULL;
#endif
  assert(ptr);
  // assert(((unsigned)ptr & (kAlignment - 1)) == 0);
  return ptr;
}

inline void AlignedFree(void* mem) {
  free(mem);
}

}  // namespace base

#endif  // MEM_H
