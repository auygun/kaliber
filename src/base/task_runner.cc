#include "task_runner.h"

namespace base {

void TaskRunner::Enqueue(base::Closure task) {
  std::unique_lock<std::mutex> scoped_lock(mutex_);
  thread_tasks_.emplace_back(std::move(task));
}

void TaskRunner::Run() {
  for (;;) {
    base::Closure task;
    {
      std::unique_lock<std::mutex> scoped_lock(mutex_);
      if (!thread_tasks_.empty()) {
        task.swap(thread_tasks_.front());
        thread_tasks_.pop_front();
      }
    }
    if (!task)
      break;
    task();
  }
}

bool TaskRunner::IsBoundToCurrentThread() {
  return thread_id_ == std::this_thread::get_id();
}

}  // namespace base
