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
}

void TaskRunner::EnqueueTaskAndReply(Location from,
                                     Closure task,
                                     Closure reply) {
  DCHECK(task);

  stack_.Push(std::make_tuple(std::move(from), std::move(task),
                              std::move(reply), &t_task_runner));
}

void TaskRunner::Run() {
  Task task;
  while (stack_.Pop(task)) {
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
