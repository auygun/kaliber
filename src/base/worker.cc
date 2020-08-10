#include "worker.h"

#include "log.h"

namespace base {

std::condition_variable Worker::cv_;
std::mutex Worker::mutex_;
std::vector<std::thread> Worker::threads_;
std::deque<std::pair<base::Closure, base::Closure>> Worker::tasks_;
bool Worker::quit_when_idle_ = false;

void Worker::Initialize(unsigned max_concurrency) {
  if (max_concurrency > std::thread::hardware_concurrency() ||
      max_concurrency == 0) {
    max_concurrency = std::thread::hardware_concurrency();
    if (max_concurrency == 0)
      max_concurrency = 1;
  }

  while (max_concurrency--)
    threads_.emplace_back(&Worker::WorkerMain);
}

void Worker::Shutdown() {
  {
    std::unique_lock<std::mutex> scoped_lock(mutex_);
    quit_when_idle_ = true;
  }
  cv_.notify_all();
  for (auto& thread : threads_)
    thread.join();
  threads_.clear();
}

void Worker::Enqueue(base::Closure task) {
  DCHECK(task);
  DCHECK(!threads_.empty());

  lock_.fetch_add(1, std::memory_order_relaxed);

  bool notify;
  {
    std::unique_lock<std::mutex> scoped_lock(mutex_);
    notify = tasks_.empty();
    tasks_.emplace_back(std::make_pair(std::move(task), [&]() -> void {
      lock_.fetch_sub(1, std::memory_order_release);
    }));
  }
  if (notify)
    cv_.notify_all();
}

void Worker::Join() {
  while (lock_.load(std::memory_order_acquire))
    ;  // spin
}

void Worker::WorkerMain() {
  for (;;) {
    std::pair<base::Closure, base::Closure> task;
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

    task.first();
    task.second();
  }
}

}  // namespace base
