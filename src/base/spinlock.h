#ifndef SPINLOCK_H
#define SPINLOCK_H

#include <atomic>

#if defined(_MSC_VER)
#include <intrin.h>
#define spinlock_pause() YieldProcessor()
#elif defined(__x86_64__) || defined(__i386__)
#include <immintrin.h>
#define spinlock_pause() _mm_pause()
#elif defined(__arm__) || defined(__aarch64__)
#define spinlock_pause() __asm__ __volatile__("yield")
#else
#define spinlock_pause()
#endif

namespace base {

class Spinlock {
 public:
  void lock() {
    for (;;) {
      // Wait for lock to be released without generating cache misses.
      bool locked = lock_.load(std::memory_order_relaxed);
      if (!locked &&
          lock_.compare_exchange_weak(locked, true, std::memory_order_acquire))
        break;
      // Reduce contention between hyper-threads.
      spinlock_pause();
    }
  }

  void unlock() { lock_.store(false, std::memory_order_release); }

 private:
  std::atomic<bool> lock_{false};
};

}  // namespace base

#endif  // SPINLOCK_H
