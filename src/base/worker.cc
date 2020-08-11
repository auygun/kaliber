#include "worker.h"

#include <condition_variable>
#include <deque>
#include <mutex>
#include <thread>
#include <utility>
#include <vector>

#include "log.h"

using Task = std::pair<base::Closure, base::Closure>;

namespace {

class ThreadPool {
 public:
  ThreadPool() = default;
  ~ThreadPool() = default;

  void Initialize(unsigned max_concurrency) {
    if (max_concurrency > std::thread::hardware_concurrency() ||
        max_concurrency == 0) {
      max_concurrency = std::thread::hardware_concurrency();
      if (max_concurrency == 0)
        max_concurrency = 1;
    }

    while (max_concurrency--)
      threads_.emplace_back(&ThreadPool::WorkerMain, this);
  }

  void Shutdown() {
    {
      std::unique_lock<std::mutex> scoped_lock(mutex_);
      quit_when_idle_ = true;
    }
    cv_.notify_all();
    for (auto& thread : threads_)
      thread.join();
    threads_.clear();
  }

  void Enqueue(Task task) {
    DCHECK(!threads_.empty());

    bool notify;
    {
      std::unique_lock<std::mutex> scoped_lock(mutex_);
      notify = tasks_.empty();
      tasks_.push_back(std::move(task));
    }
    if (notify)
      cv_.notify_all();
  }

 private:
  std::condition_variable cv_;
  std::mutex mutex_;
  std::vector<std::thread> threads_;
  std::deque<Task> tasks_;
  bool quit_when_idle_;

  void WorkerMain() {
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
};

ThreadPool g_thread_pool;

}  // namespace

namespace base {

Worker::Worker() = default;

Worker::~Worker() {
  DCHECK(lock_.load(std::memory_order_acquire) == 0);
}

void Worker::Initialize(unsigned max_concurrency) {
  g_thread_pool.Initialize(max_concurrency);
}

void Worker::Shutdown() {
  g_thread_pool.Shutdown();
}

void Worker::Enqueue(base::Closure task) {
  DCHECK(task);

  lock_.fetch_add(1, std::memory_order_relaxed);
  g_thread_pool.Enqueue(std::make_pair(std::move(task), [&]() -> void {
    lock_.fetch_sub(1, std::memory_order_release);
  }));
}

void Worker::Join() {
  while (lock_.load(std::memory_order_acquire))
    ;  // spin
}

}  // namespace base
