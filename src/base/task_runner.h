#ifndef BASE_TASK_RUNNER_H
#define BASE_TASK_RUNNER_H

#include <atomic>
#include <deque>
#include <memory>
#include <mutex>
#include <tuple>

#include "base/closure.h"

namespace base {

namespace internal {

// Adapted from Chromium project.
// Adapts a function that produces a result via a return value to
// one that returns via an output parameter.
template <typename ReturnType>
void ReturnAsParamAdapter(std::function<ReturnType()> func,
                          ReturnType* result) {
  *result = func();
}

// Adapts a ReturnType* result to a callblack that expects a ReturnType.
template <typename ReturnType>
void ReplyAdapter(std::function<void(ReturnType)> callback,
                  ReturnType* result) {
  callback(std::move(*result));
  delete result;
}

}  // namespace internal

// Runs queued tasks (in the form of Closure objects). All methods are
// thread-safe and can be called on any thread.
// Tasks run in FIFO order when consumed by a single thread. When consumed
// concurrently by multiple threads, it doesn't guarantee whether tasks overlap,
// or whether they run on a particular thread.
class TaskRunner {
 public:
  TaskRunner() = default;
  ~TaskRunner() = default;

  void PostTask(Location from, Closure task);

  void PostTaskAndReply(Location from, Closure task, Closure reply);

  template <typename ReturnType>
  void PostTaskAndReplyWithResult(Location from,
                                  std::function<ReturnType()> task,
                                  std::function<void(ReturnType)> reply) {
    auto* result = new ReturnType;
    return PostTaskAndReply(
        from,
        std::bind(internal::ReturnAsParamAdapter<ReturnType>, std::move(task),
                  result),
        std::bind(internal::ReplyAdapter<ReturnType>, std::move(reply),
                  result));
  }

  void MultiConsumerRun();
  void SingleConsumerRun();

  void CancelTasks();
  void WaitForCompletion();

  static void CreateThreadLocalTaskRunner();
  static std::shared_ptr<TaskRunner> GetThreadLocalTaskRunner();

 private:
  using Task = std::tuple<Location, Closure>;

  std::deque<Task> queue_;
  mutable std::mutex lock_;
  std::atomic<size_t> task_count_{0};
  std::atomic<bool> cancel_tasks_{false};

  static thread_local std::shared_ptr<TaskRunner> thread_local_task_runner;

  void CancelTasksInternal();

  TaskRunner(TaskRunner const&) = delete;
  TaskRunner& operator=(TaskRunner const&) = delete;
};

}  // namespace base

#endif  // BASE_TASK_RUNNER_H
