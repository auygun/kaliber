#ifndef WORKER_H
#define WORKER_H

#include <condition_variable>
#include <deque>
#include <mutex>
#include <thread>
#include <vector>
#include "closure.h"

namespace base {

// Feed the worker tasks and they will be called on a thread from the pool.
class Worker {
 public:
  Worker(unsigned max_concurrency = 0);
  ~Worker();

  void Enqueue(base::Closure task);
  void Join();

 private:
  bool active_ = false;
  unsigned max_concurrency_ = 0;

  std::condition_variable cv_;
  std::mutex mutex_;
  std::vector<std::thread> threads_;
  std::deque<base::Closure> tasks_;
  bool quit_when_idle_ = false;

  void WorkerMain();

  Worker(Worker const&) = delete;
  Worker& operator=(Worker const&) = delete;
};

}  // namespace base

#endif  // WORKER_H
