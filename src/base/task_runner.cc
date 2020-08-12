#include "task_runner.h"

#include "log.h"

namespace base {

void TaskRunner::Enqueue(Location from, Closure task, Closure done_cb) {
  DCHECK(task);

  bool notify;
  {
    std::unique_lock<std::mutex> scoped_lock(mutex_);
    notify = blocking_ && tasks_.empty();
    tasks_.emplace_back(std::move(from), std::move(task), std::move(done_cb));
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
      if (tasks_.empty())
        return;
      task.swap(tasks_.front());
      tasks_.pop_front();
    }

    auto [from, task_cb, done_cb] = task;

#if 0
    DLOG << "Task from: " << LOCATION(from);
#endif

    task_cb();

    if (done_cb)
      done_cb();
  }
}

void TaskRunner::QuitWhenIdle() {
  DCHECK(blocking_);

  {
    std::unique_lock<std::mutex> scoped_lock(mutex_);
    quit_when_idle_ = true;
  }
  cv_.notify_all();
}

}  // namespace base
