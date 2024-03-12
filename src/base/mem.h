#ifndef BASE_MEM_H
#define BASE_MEM_H

#include <stddef.h>
#include <stdint.h>
#include <memory>
#include <span>

#if defined(_WIN32)
#include <malloc.h>
#else
#include <stdlib.h>
#endif

#include "base/log.h"
#include "base/misc.h"

namespace base {

inline void AlignedFree(void* mem) {
#if defined(_WIN32)
  _aligned_free(mem);
#else
  free(mem);
#endif
}

namespace internal {

struct ScopedAlignedFree {
  inline void operator()(void* x) const { AlignedFree(x); }
};

}  // namespace internal

template <typename T>
using AlignedMemPtr = std::unique_ptr<T, internal::ScopedAlignedFree>;

void* AlignedAlloc(size_t size, size_t alignment);

void* AlignedRealloc(void* ptr,
                     size_t old_size,
                     size_t new_size,
                     size_t alignment);

inline bool IsAligned(const void* val, size_t alignment) {
  DCHECK(IsPow2(alignment)) << alignment << " is not a power of 2";
  return (reinterpret_cast<uintptr_t>(val) & (alignment - 1)) == 0;
}

// Utilities for stack allocation.
#define ALLOCA(size) (size != 0 ? alloca(size) : nullptr)
#define ALLOCA_ARRAY(type, count) ((type*)ALLOCA(sizeof(type) * count))
#define ALLOCA_SINGLE(type) ALLOCA_ARRAY(type, 1)
#define ALLOCA_SPAN(type, count) std::span(ALLOCA_ARRAY(type, count), count)

}  // namespace base

#endif  // BASE_MEM_H
