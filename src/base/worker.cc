#include "worker.h"
#include "log.h"

namespace base {

Worker::Worker(unsigned max_concurrency) : max_concurrency_(max_concurrency) {
  if (max_concurrency_ > std::thread::hardware_concurrency() ||
      max_concurrency_ == 0) {
    max_concurrency_ = std::thread::hardware_concurrency();
    if (max_concurrency_ == 0)
      max_concurrency_ = 1;
  }
}

Worker::~Worker() = default;

void Worker::Enqueue(base::Closure task) {
  if (!active_) {
    unsigned concurrency = max_concurrency_;
    while (concurrency--)
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
    base::Closure task;
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

}  // namespace base
