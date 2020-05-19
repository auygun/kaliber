#ifndef WORKER_H
#define WORKER_H

#include "callback.h"
#include <condition_variable>
#include <deque>
#include <mutex>
#include <thread>
#include <vector>

namespace base {

// Feed the worker tasks and they will be called on a thread from the pool.
class Worker {
 public:
  Worker() = default;
  ~Worker() = default;

  void Enqueue(base::Callback task);
  void Join();

 private:
  bool active_ = false;

  std::condition_variable cv_;
  std::mutex mutex_;
  std::vector<std::thread> threads_;
  std::deque<base::Callback> tasks_;
  bool quit_when_idle_ = false;

  void WorkerMain();
  
  Worker(Worker const&) = delete;
  Worker& operator=(Worker const&) = delete;
};

} // namespace eng

#endif // WORKER_H
