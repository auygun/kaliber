#include "task_runner.h"

#include "log.h"

namespace {

thread_local base::TaskRunner t_task_runner;

}  // namespace

namespace base {

TaskRunner& TaskRunner::GetLocalTaskRunner() {
  return t_task_runner;
}

void TaskRunner::EnqueueTask(Location from, Closure task) {
  DCHECK(task);

  std::lock_guard<std::mutex> scoped_lock(lock_);
  queue_.emplace_back(std::move(from), std::move(task), nullptr, nullptr);
}

void TaskRunner::EnqueueTaskAndReply(Location from,
                                     Closure task,
                                     Closure reply) {
  DCHECK(task);

  std::lock_guard<std::mutex> scoped_lock(lock_);
  queue_.emplace_back(std::move(from), std::move(task), std::move(reply),
                      &t_task_runner);
}

bool TaskRunner::Enmpty() const {
  std::lock_guard<std::mutex> scoped_lock(lock_);
  return queue_.empty();
}

void TaskRunner::Run() {
  for (;;) {
    Task task;
    {
      std::lock_guard<std::mutex> scoped_lock(lock_);
      if (queue_.empty())
        return;
      task.swap(queue_.front());
      queue_.pop_front();
    }

    auto [from, task_cb, reply_cb, reply_tr] = task;

#if 0
    LOG << __func__ << " from: " << LOCATION(from);
#endif

    task_cb();

    if (reply_cb)
      reply_tr->EnqueueTask(from, reply_cb);
  }
}

}  // namespace base
