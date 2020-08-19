#ifndef WORKER_H
#define WORKER_H

#include <condition_variable>
#include <mutex>

#include "closure.h"

namespace base {

class TaskRunner;

// Feed the worker tasks (in the form of Closure objects) and they will be
// called on any thread from the pool.
class Worker {
 public:
  Worker();
  ~Worker();

  // Initialize the global thread pool.
  static void Initialize(unsigned max_concurrency = 0);

  // Shutdown the global thread pool.
  static void Shutdown();

  // Access to task runner of the global thread pool.
  static TaskRunner& GetTaskRunner();

  // Enqueue a task to be called on any thread from the pool.
  void Enqueue(Location from, Closure task);

  // Wait for the tasks to complete.
  void Join();

 private:
  // Semaphore.
  std::mutex mutex_;
  std::condition_variable cv_;
  int count_ = 0;

  Worker(Worker const&) = delete;
  Worker& operator=(Worker const&) = delete;
};

}  // namespace base

#endif  // WORKER_H
