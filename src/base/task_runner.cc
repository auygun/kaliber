#include "base/task_runner.h"

#include <thread>

#include "base/log.h"

namespace base {

namespace {

void PostTaskAndReplyRelay(Location from,
                           Closure task_cb,
                           Closure reply_cb,
                           std::shared_ptr<TaskRunner> destination) {
  task_cb();

  if (reply_cb)
    destination->PostTask(from, std::move(reply_cb));
}

}  // namespace

// The task runner that belongs to the thread it's created in. Tasks to be run
// on a specific thread can be posted to this task runner.
// TaskRunner::GetThreadLocalTaskRunner()->SingleConsumerRun() is expected to be
// periodically called.
thread_local std::shared_ptr<TaskRunner> TaskRunner::thread_local_task_runner;

void TaskRunner::CreateThreadLocalTaskRunner() {
  DCHECK(!thread_local_task_runner);

  thread_local_task_runner = std::make_shared<TaskRunner>();
}

std::shared_ptr<TaskRunner> TaskRunner::GetThreadLocalTaskRunner() {
  return thread_local_task_runner;
}

void TaskRunner::PostTask(Location from, Closure task) {
  DCHECK(task) << LOCATION(from);

  task_count_.fetch_add(1, std::memory_order_relaxed);
  std::lock_guard<std::mutex> scoped_lock(lock_);
  queue_.emplace_back(from, std::move(task));
}

void TaskRunner::PostTaskAndReply(Location from, Closure task, Closure reply) {
  DCHECK(task) << LOCATION(from);
  DCHECK(reply) << LOCATION(from);
  DCHECK(thread_local_task_runner) << LOCATION(from);

  auto relay = std::bind(PostTaskAndReplyRelay, from, std::move(task),
                         std::move(reply), thread_local_task_runner);
  PostTask(from, std::move(relay));
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
    task_count_.fetch_sub(1, std::memory_order_release);

    if (cancel_tasks_.load(std::memory_order_relaxed)) {
      CancelTasksInternal();
      break;
    }
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
    task_count_.fetch_sub(1, std::memory_order_release);

    if (cancel_tasks_.load(std::memory_order_relaxed)) {
      CancelTasksInternal();
      break;
    }
  }
}

void TaskRunner::CancelTasks() {
  cancel_tasks_.store(true, std::memory_order_relaxed);
}

void TaskRunner::WaitForCompletion() {
  while (task_count_.load(std::memory_order_acquire) > 0)
    std::this_thread::yield();
}

void TaskRunner::CancelTasksInternal() {
  cancel_tasks_.store(false, std::memory_order_relaxed);
  task_count_.store(0, std::memory_order_relaxed);
  std::lock_guard<std::mutex> scoped_lock(lock_);
  queue_.clear();
}

}  // namespace base
