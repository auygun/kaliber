#ifndef BASE_SEMAPHORE_H
#define BASE_SEMAPHORE_H

#include <condition_variable>
#include <mutex>

#include "base/log.h"

namespace base {

class Semaphore {
 public:
  Semaphore(int count = 0) : count_(count) {}

  void Acquire() {
    std::unique_lock<std::mutex> scoped_lock(mutex_);
    cv_.wait(scoped_lock, [&]() { return count_ > 0; });
    --count_;
    DCHECK(count_ >= 0);
  }

  void Release() {
    {
      std::lock_guard<std::mutex> scoped_lock(mutex_);
      ++count_;
    }
    cv_.notify_one();
  }

 private:
  std::condition_variable cv_;
  std::mutex mutex_;
  int count_ = 0;

  Semaphore(Semaphore const&) = delete;
  Semaphore& operator=(Semaphore const&) = delete;
};

}  // namespace base

#endif  // BASE_SEMAPHORE_H
