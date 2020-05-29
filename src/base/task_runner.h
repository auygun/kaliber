#ifndef TASK_RUNNER_H
#define TASK_RUNNER_H

#include <deque>
#include <mutex>
#include <thread>
#include "closure.h"

namespace base {

class TaskRunner {
 public:
  TaskRunner() = default;
  ~TaskRunner() = default;

  void Enqueue(base::Closure cb);
  void Run();

  bool IsBoundToCurrentThread();

 private:
  std::thread::id thread_id_ = std::this_thread::get_id();
  std::mutex mutex_;
  std::deque<base::Closure> thread_tasks_;

  TaskRunner(TaskRunner const&) = delete;
  TaskRunner& operator=(TaskRunner const&) = delete;
};

}  // namespace base

#endif  // TASK_RUNNER_H
