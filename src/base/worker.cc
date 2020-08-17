#include "worker.h"

#include <thread>
#include <vector>

#include "log.h"
#include "task_runner.h"

namespace {

class ThreadPool {
 public:
  ThreadPool() = default;

  ~ThreadPool() { Shutdown(); }

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
    task_runner_.QuitWhenIdle();

    for (auto& thread : threads_)
      thread.join();
    threads_.clear();
  }

  base::TaskRunner& GetTaskRunner() { return task_runner_; }

 private:
  std::vector<std::thread> threads_;
  base::TaskRunner task_runner_{true};

  void WorkerMain() { task_runner_.Run(); }
};

ThreadPool g_thread_pool;

}  // namespace

namespace base {

Worker::Worker() = default;

Worker::~Worker() {
  DCHECK(lock_.load(std::memory_order_acquire) == 0) << "Join must be called.";
}

void Worker::Initialize(unsigned max_concurrency) {
  g_thread_pool.Initialize(max_concurrency);
}

void Worker::Shutdown() {
  g_thread_pool.Shutdown();
}

TaskRunner& Worker::GetTaskRunner() {
  return g_thread_pool.GetTaskRunner();
}

void Worker::Enqueue(Location from, Closure task) {
  DCHECK(task);

  if (lock_.fetch_add(1, std::memory_order_relaxed) == 0)
    mutex_.lock();

  g_thread_pool.GetTaskRunner().Enqueue(from, std::move(task), [&]() -> void {
    if (lock_.fetch_sub(1, std::memory_order_release) - 1 == 0)
      mutex_.unlock();
  });
}

void Worker::Join() {
  // Wait for the tasks to complete.
  mutex_.lock();
}

}  // namespace base
