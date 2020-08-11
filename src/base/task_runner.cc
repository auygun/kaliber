#include "task_runner.h"

#include "log.h"

namespace base {

void TaskRunner::Enqueue(base::Closure task, base::Closure done_cb) {
  DCHECK(task);

  bool notify;
  {
    std::unique_lock<std::mutex> scoped_lock(mutex_);
    notify = blocking_ && tasks_.empty();
    tasks_.emplace_back(std::make_pair(std::move(task), std::move(done_cb)));
  }
  if (notify)
    cv_.notify_all();
}

void TaskRunner::Run() {
  for (;;) {
    Task task;
    {
      std::unique_lock<std::mutex> scoped_lock(mutex_);
      while (blocking_ && tasks_.empty()) {
        if (quit_when_idle_)
          return;
        cv_.wait(scoped_lock);
      }
      if (!tasks_.empty()) {
        task.swap(tasks_.front());
        tasks_.pop_front();
      }
    }

    if (!task.first) {
      DCHECK(!blocking_);
      break;
    }

    task.first();

    if (task.second)
      task.second();
  }
}

void TaskRunner::QuitWhenIdle() {
  {
    std::unique_lock<std::mutex> scoped_lock(mutex_);
    quit_when_idle_ = true;
  }
  cv_.notify_all();
}

}  // namespace base
