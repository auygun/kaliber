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

  stack_.Push(
      std::make_tuple(std::move(from), std::move(task), nullptr, nullptr));

  if (delegate_)
    delegate_->Signal();
}

void TaskRunner::EnqueueTaskAndReply(Location from,
                                     Closure task,
                                     Closure reply) {
  DCHECK(task);

  stack_.Push(std::make_tuple(std::move(from), std::move(task),
                              std::move(reply), &t_task_runner));

  if (delegate_)
    delegate_->Signal();
}

void TaskRunner::Run() {
  Task task;
  while (stack_.Pop(task)) {
    auto [from, task_cb, reply_cb, reply_tr] = task;

    task_cb();

    if (reply_cb)
      reply_tr->EnqueueTask(from, reply_cb);
  }
}

}  // namespace base
