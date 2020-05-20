#ifndef TASK_RUNNER_H
#define TASK_RUNNER_H

#include <deque>
#include <mutex>
#include <thread>
#include "callback.h"

namespace base {

class TaskRunner {
 public:
  TaskRunner() = default;
  ~TaskRunner() = default;

  void Enqueue(base::Callback cb);
  void Run();

  bool IsBoundToCurrentThread();

 private:
  std::thread::id thread_id_ = std::this_thread::get_id();
  std::mutex mutex_;
  std::deque<base::Callback> main_thread_tasks_;
};

}  // namespace base

#endif  // TASK_RUNNER_H
