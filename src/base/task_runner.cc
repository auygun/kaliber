#include "task_runner.h"

namespace base {

void TaskRunner::Enqueue(base::Callback task) {
  std::unique_lock<std::mutex> scoped_lock(mutex_);
  main_thread_tasks_.emplace_back(std::move(task));
}

void TaskRunner::Run() {
  for (;;) {
    base::Callback task;
    {
      std::unique_lock<std::mutex> scoped_lock(mutex_);
      if (!main_thread_tasks_.empty()) {
        task.swap(main_thread_tasks_.front());
        main_thread_tasks_.pop_front();
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
