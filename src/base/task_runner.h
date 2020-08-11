#ifndef TASK_RUNNER_H
#define TASK_RUNNER_H

#include <condition_variable>
#include <deque>
#include <mutex>
#include <tuple>

#include "closure.h"

namespace base {

// Runs queued tasks (in the form of Closure objects). All methods are
// thread-safe and can be called on any thread.
// When used in a thread pool, TaskRunner does not guarantee the order in which
// tasks are run, whether tasks overlap, or whether they run on a particular
// thread.
class TaskRunner {
 public:
  using Task = std::tuple<Location, base::Closure, base::Closure>;

  // If blocking is true, Run method won't return until QuitWhenIdle is called.
  TaskRunner(bool blocking = false) : blocking_(blocking) {}
  ~TaskRunner() = default;

  // Enqueue the given task to be run. On completion, done_cb is called on the
  // same thread that ran the task.
  void Enqueue(Location from,
               base::Closure task,
               base::Closure done_cb = nullptr);

  // Run all queued tasks and return upon completion if non-blocking. Otherwise,
  // wait for more tasks to run.
  void Run();

  // Tell Run method to stop waiting for tasks and return.
  void QuitWhenIdle();

 private:
  std::condition_variable cv_;
  std::mutex mutex_;
  std::deque<Task> tasks_;
  bool quit_when_idle_ = false;

  const bool blocking_;

  TaskRunner(TaskRunner const&) = delete;
  TaskRunner& operator=(TaskRunner const&) = delete;
};

}  // namespace base

#endif  // TASK_RUNNER_H
