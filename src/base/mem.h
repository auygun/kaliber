#ifndef MEM_H
#define MEM_H

#include <assert.h>
#include <stdlib.h>

#if defined(__ANDROID__)
#include <malloc.h>
#endif

inline void *AlignedAlloc(size_t size) {
  const size_t kAlignment = 16;

  void* ptr = NULL;
#if defined(__ANDROID__)
  ptr = memalign(kAlignment, size);
#else
  if (posix_memalign(&ptr, kAlignment, size))
    ptr = NULL;
#endif
  assert(ptr);
  //assert(((unsigned)ptr & (kAlignment - 1)) == 0);
  return ptr;
}

#define AlignedFree(mem)   free(mem)

#define ALIGN_MEM(alignment) __attribute__((aligned(alignment)))

#endif // MEM_H
