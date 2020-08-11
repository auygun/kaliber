#ifndef WORKER_H
#define WORKER_H

#include <atomic>

#include "closure.h"

namespace base {

// Feed the worker tasks and they will be called on a thread from the pool.
class Worker {
 public:
  Worker();
  ~Worker();

  static void Initialize(unsigned max_concurrency = 0);
  static void Shutdown();

  void Enqueue(base::Closure task);
  void Join();

 private:
  std::atomic<int> lock_ = 0;

  Worker(Worker const&) = delete;
  Worker& operator=(Worker const&) = delete;
};

}  // namespace base

#endif  // WORKER_H
