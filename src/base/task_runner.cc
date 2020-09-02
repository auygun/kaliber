#include "task_runner.h"

#include "log.h"

namespace {

void EnqueueTaskAndReplyRelay(const base::Location& from,
                              base::Closure task_cb,
                              base::Closure reply_cb,
                              base::TaskRunner* destination) {
  task_cb();

  if (reply_cb)
    destination->EnqueueTask(from, std::move(reply_cb));
}

}  // namespace

namespace base {

thread_local std::unique_ptr<TaskRunner> TaskRunner::thread_local_task_runner;

void TaskRunner::CreateThreadLocalTaskRunner() {
  DCHECK(!thread_local_task_runner);

  thread_local_task_runner = std::make_unique<TaskRunner>();
}

TaskRunner* TaskRunner::GetThreadLocalTaskRunner() {
  return thread_local_task_runner.get();
}

void TaskRunner::EnqueueTask(const Location& from, Closure task) {
  DCHECK(task) << LOCATION(from);

  std::lock_guard<std::mutex> scoped_lock(lock_);
  queue_.emplace_back(from, std::move(task));
}

void TaskRunner::EnqueueTaskAndReply(const Location& from,
                                     Closure task,
                                     Closure reply) {
  DCHECK(task) << LOCATION(from);
  DCHECK(reply) << LOCATION(from);
  DCHECK(thread_local_task_runner) << LOCATION(from);

  auto relay = std::bind(::EnqueueTaskAndReplyRelay, from, std::move(task),
                         std::move(reply), thread_local_task_runner.get());

  std::lock_guard<std::mutex> scoped_lock(lock_);
  queue_.emplace_back(from, std::move(relay));
}

void TaskRunner::MultiConsumerRun() {
  for (;;) {
    Task task;
    {
      std::lock_guard<std::mutex> scoped_lock(lock_);
      if (queue_.empty())
        return;
      task.swap(queue_.front());
      queue_.pop_front();
    }

    auto [from, task_cb] = task;

#if 0
    LOG << __func__ << " from: " << LOCATION(from);
#endif

    task_cb();
  }
}

void TaskRunner::SingleConsumerRun() {
  std::deque<Task> queue;
  {
    std::lock_guard<std::mutex> scoped_lock(lock_);
    if (queue_.empty())
      return;
    queue.swap(queue_);
  }

  while (!queue.empty()) {
    auto [from, task_cb] = queue.front();
    queue.pop_front();

#if 0
    LOG << __func__ << " from: " << LOCATION(from);
#endif

    task_cb();
  }
}

}  // namespace base
