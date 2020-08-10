#ifndef WORKER_H
#define WORKER_H

#include <atomic>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <thread>
#include <utility>
#include <vector>
#include "closure.h"

namespace base {

// Feed the worker tasks and they will be called on a thread from the pool.
class Worker {
 public:
  Worker() = default;
  ~Worker() = default;

  static void Initialize(unsigned max_concurrency = 0);
  static void Shutdown();

  void Enqueue(base::Closure task);
  void Join();

 private:
  std::atomic<int> lock_ = 0;

  static std::condition_variable cv_;
  static std::mutex mutex_;
  static std::vector<std::thread> threads_;
  static std::deque<std::pair<base::Closure, base::Closure>> tasks_;
  static bool quit_when_idle_;

  static void WorkerMain();

  Worker(Worker const&) = delete;
  Worker& operator=(Worker const&) = delete;
};

}  // namespace base

#endif  // WORKER_H
