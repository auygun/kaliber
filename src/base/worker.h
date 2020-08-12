#ifndef WORKER_H
#define WORKER_H

#include <atomic>

#include "closure.h"

namespace base {

class TaskRunner;

// Feed the worker tasks (in the form of Closure objects) and they will be
// called on any thread from the pool. Call Join method to synchronize with
// successor tasks.
// GetTaskRunner method can be used to access to the task runner and enqueue
// fire-and-forget tasks (i.e. no synchronization).
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
  std::atomic<int> lock_ = 0;

  Worker(Worker const&) = delete;
  Worker& operator=(Worker const&) = delete;
};

}  // namespace base

#endif  // WORKER_H
