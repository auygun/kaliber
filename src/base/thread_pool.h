#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <condition_variable>
#include <mutex>
#include <thread>
#include <vector>

#include "closure.h"
#include "task_runner.h"

namespace base {

class TaskRunner;

// Feed the ThreadPool tasks (in the form of Closure objects) and they will be
// called on any thread from the pool.
class ThreadPool {
 public:
  ThreadPool();
  ~ThreadPool();

  static ThreadPool& Get() { return *singleton; }

  void Initialize(unsigned max_concurrency = 0);

  void Shutdown();

  void EnqueueTask(Location from, Closure task);

  void EnqueueTaskAndReply(Location from, Closure task, Closure reply);

 private:
  std::vector<std::thread> threads_;

  std::condition_variable cv_;
  std::mutex mutex_;
  bool wake_up_ = false;
  bool quit_when_idle_ = false;

  base::TaskRunner task_runner_;

  static ThreadPool* singleton;

  void WakeUpOne();

  void WorkerMain();

  ThreadPool(ThreadPool const&) = delete;
  ThreadPool& operator=(ThreadPool const&) = delete;
};

}  // namespace base

#endif  // THREAD_POOL_H
