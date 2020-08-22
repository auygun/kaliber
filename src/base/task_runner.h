#ifndef TASK_RUNNER_H
#define TASK_RUNNER_H

#include <tuple>

#include "closure.h"
#include "concurrent_stack.h"

namespace base {

// Runs queued tasks (in the form of Closure objects). All methods are
// thread-safe and can be called on any thread.
// Tasks run in LIFO order. When consumed concurrently by multiple threads, it
// doesn't guarantee whether tasks overlap, or whether they run on a particular
// thread.
class TaskRunner {
 public:
  class Delegate {
   public:
    virtual void Signal() = 0;
  };

  TaskRunner() = default;
  ~TaskRunner() = default;

  void SetDelegate(Delegate* delegate) { delegate_ = delegate; }

  void EnqueueTask(Location from, Closure task);

  void EnqueueTaskAndReply(Location from, Closure task, Closure done_cb);

  void Run();

  bool Enmpty() const { return stack_.Empty(); }

  static TaskRunner& GetLocalTaskRunner();

 private:
  using Task = std::tuple<Location, Closure, Closure, TaskRunner*>;

  ConcurrentStack<Task> stack_;

  Delegate* delegate_ = nullptr;

  TaskRunner(TaskRunner const&) = delete;
  TaskRunner& operator=(TaskRunner const&) = delete;
};

}  // namespace base

#endif  // TASK_RUNNER_H
