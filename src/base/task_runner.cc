#include "base/task_runner.h"

#include <thread>

#include "base/log.h"

namespace base {

namespace {

void PostTaskAndReplyRelay(Location from,
                           Closure task_cb,
                           Closure reply_cb,
                           std::shared_ptr<TaskRunner> destination,
                           bool front) {
  task_cb();

  if (reply_cb)
    destination->PostTask(from, std::move(reply_cb), front);
}

}  // namespace

// The task runner that belongs to the thread it's created in. Tasks to be run
// on a specific thread can be posted to this task runner.
// TaskRunner::GetThreadLocalTaskRunner()->RunTasks() is expected to be
// periodically called.
thread_local std::shared_ptr<TaskRunner> TaskRunner::thread_local_task_runner;

void TaskRunner::CreateThreadLocalTaskRunner() {
  DCHECK(!thread_local_task_runner);

  thread_local_task_runner = std::make_shared<TaskRunner>();
}

std::shared_ptr<TaskRunner> TaskRunner::GetThreadLocalTaskRunner() {
  return thread_local_task_runner;
}

void TaskRunner::PostTask(Location from, Closure task, bool front) {
  DCHECK(task) << LOCATION(from);

  task_count_.fetch_add(1, std::memory_order_relaxed);
  std::lock_guard<std::mutex> scoped_lock(lock_);
  if (front)
    queue_.emplace_front(from, std::move(task));
  else
    queue_.emplace_back(from, std::move(task));
}

void TaskRunner::PostTaskAndReply(Location from,
                                  Closure task,
                                  Closure reply,
                                  bool front) {
  DCHECK(task) << LOCATION(from);
  DCHECK(reply) << LOCATION(from);
  DCHECK(thread_local_task_runner) << LOCATION(from);

  auto relay = std::bind(PostTaskAndReplyRelay, from, std::move(task),
                         std::move(reply), thread_local_task_runner, front);
  PostTask(from, std::move(relay), front);
}

void TaskRunner::CancelTasks() {
  std::lock_guard<std::mutex> scoped_lock(lock_);
  task_count_.fetch_sub(queue_.size(), std::memory_order_release);
  queue_.clear();
}

void TaskRunner::WaitForCompletion() {
  while (task_count_.load(std::memory_order_acquire) > 0)
    std::this_thread::yield();
}

template <>
void TaskRunner::RunTasks<Consumer::Multi>() {
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
    LOG(0) << __func__ << " from: " << LOCATION(from);
#endif

    task_cb();
    task_count_.fetch_sub(1, std::memory_order_release);
  }
}

template <>
void TaskRunner::RunTasks<Consumer::Single>() {
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
  }
}

template <>
void TaskRunner::RunTasks<Consumer::NoBlocking>() {
  std::deque<Task> queue;
  {
    std::unique_lock<std::mutex> scoped_lock(lock_, std::try_to_lock);
    if (scoped_lock) {
      if (queue_.empty())
        return;
      queue.swap(queue_);
    }
  }

  while (!queue.empty()) {
    auto [from, task_cb] = queue.front();
    queue.pop_front();

#if 0
    LOG << __func__ << " from: " << LOCATION(from);
#endif

    task_cb();
    task_count_.fetch_sub(1, std::memory_order_release);
  }
}

}  // namespace base
