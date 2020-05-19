#include "worker.h"
#include "log.h"

namespace base {

void Worker::Enqueue(base::Callback task) {
  if (!active_) {
    unsigned supported = std::thread::hardware_concurrency();
    if (supported == 0)
      supported = 1;
    while (supported--)
      threads_.emplace_back(&Worker::WorkerMain, this);
    active_ = true;
  }

  bool notify;
  {
    std::unique_lock<std::mutex> scoped_lock(mutex_);
    notify = tasks_.empty();
    tasks_.emplace_back(std::move(task));
  }
  if (notify)
    cv_.notify_all();
}

void Worker::Join() {
  if (!active_)
    return;

  {
    std::unique_lock<std::mutex> scoped_lock(mutex_);
    quit_when_idle_ = true;
  }
  cv_.notify_all();
  for (auto& thread : threads_)
    thread.join();
  threads_.clear();
  active_ = false;
}

void Worker::WorkerMain() {
  for (;;) {
    base::Callback task;
    {
      std::unique_lock<std::mutex> scoped_lock(mutex_);
      while (tasks_.empty()) {
        if (quit_when_idle_)
          return;
        cv_.wait(scoped_lock);
      }
      task.swap(tasks_.front());
      tasks_.pop_front();
    }

    task();
  }
}

} // namespace eng
