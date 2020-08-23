#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include "closure.h"

namespace base {

class TaskRunner;

// Feed the TaskRunner tasks (in the form of Closure objects) and they will be
// called on any thread from the pool.
class ThreadPool {
 public:
  static void Initialize(unsigned max_concurrency = 0);

  static void Shutdown();

  static TaskRunner& GetTaskRunner();

 private:
  ThreadPool() = delete;
  ~ThreadPool() = delete;

  ThreadPool(ThreadPool const&) = delete;
  ThreadPool& operator=(ThreadPool const&) = delete;
};

}  // namespace base

#endif  // THREAD_POOL_H
