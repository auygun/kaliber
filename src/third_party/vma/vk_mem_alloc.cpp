#if defined(__ANDROID__)
#define VK_NO_PROTOTYPES 1
#pragma clang diagnostic ignored "-Wunused-variable"
#endif
#define VMA_IMPLEMENTATION
#pragma clang diagnostic ignored "-Wnullability-completeness"
#include "vk_mem_alloc.h"
