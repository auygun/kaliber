#ifndef TASK_RUNNER_H
#define TASK_RUNNER_H

#include <deque>
#include <mutex>
#include <tuple>

#include "closure.h"

namespace base {

// Runs queued tasks (in the form of Closure objects). All methods are
// thread-safe and can be called on any thread.
// Tasks run in FIFO order. When consumed concurrently by multiple threads, it
// doesn't guarantee whether tasks overlap, or whether they run on a particular
// thread.
class TaskRunner {
 public:
  TaskRunner() = default;
  ~TaskRunner() = default;

  void EnqueueTask(Location from, Closure task);

  void EnqueueTaskAndReply(Location from, Closure task, Closure reply);

  void Run();

  bool Enmpty() const;

  static TaskRunner& GetLocalTaskRunner();

 private:
  using Task = std::tuple<Location, Closure, Closure, TaskRunner*>;

  std::deque<Task> queue_;
  mutable std::mutex lock_;

  TaskRunner(TaskRunner const&) = delete;
  TaskRunner& operator=(TaskRunner const&) = delete;
};

}  // namespace base

#endif  // TASK_RUNNER_H
