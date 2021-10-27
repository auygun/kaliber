#ifndef BASE_THREAD_POOL_H
#define BASE_THREAD_POOL_H

#include <atomic>
#include <thread>
#include <vector>

#include "base/closure.h"
#include "base/semaphore.h"
#include "base/task_runner.h"

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

  void PostTask(const Location& from, Closure task);

  void PostTaskAndReply(const Location& from, Closure task, Closure reply);

  template <typename ReturnType>
  void PostTaskAndReplyWithResult(const Location& from,
                                  std::function<ReturnType()> task,
                                  std::function<void(ReturnType)> reply) {
    task_runner_.PostTaskAndReplyWithResult(from, std::move(task),
                                            std::move(reply));
    semaphore_.Release();
  }

 private:
  std::vector<std::thread> threads_;

  Semaphore semaphore_;
  std::atomic<bool> quit_{false};

  base::TaskRunner task_runner_;

  static ThreadPool* singleton;

  void WorkerMain();

  ThreadPool(ThreadPool const&) = delete;
  ThreadPool& operator=(ThreadPool const&) = delete;
};

}  // namespace base

#endif  // BASE_THREAD_POOL_H
