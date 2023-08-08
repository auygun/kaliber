#include "base/mem.h"

#include <cstring>

#if defined(__ANDROID__)
#include <malloc.h>
#endif

namespace base {

void* AlignedAlloc(size_t size, size_t alignment) {
  DCHECK(size > 0U);
  DCHECK(IsPow2(alignment));
  DCHECK((alignment % sizeof(void*)) == 0U);

  void* ptr = nullptr;
#if defined(_WIN32)
  ptr = _aligned_malloc(size, alignment);
#elif defined(__ANDROID__)
  ptr = memalign(alignment, size);
#else
  int ret = posix_memalign(&ptr, alignment, size);
  if (ret != 0) {
    DLOG(0) << "posix_memalign() returned with error " << ret;
    ptr = nullptr;
  }
#endif

  // Aligned allocations may fail for non-memory related reasons.
  CHECK(ptr) << "Aligned allocation failed. "
             << "size=" << size << ", alignment=" << alignment;
  DCHECK(IsAligned(ptr, alignment));
  return ptr;
}

void* AlignedRealloc(void* ptr,
                     size_t old_size,
                     size_t new_size,
                     size_t alignment) {
  auto* new_ptr = AlignedAlloc(new_size, alignment);
  memmove(new_ptr, ptr, old_size);
  AlignedFree(ptr);
  return new_ptr;
}

}  // namespace base
