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
  DCHECK([&]() -> bool {
    std::unique_lock<std::mutex> scoped_lock(mutex_);
    return !count_;
  }()) << "Join must be called.";
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

  {
    std::unique_lock<std::mutex> scoped_lock(mutex_);
    ++count_;
  }

  g_thread_pool.GetTaskRunner().Enqueue(from, std::move(task), [&]() -> void {
    bool notify;
    {
      std::unique_lock<std::mutex> scoped_lock(mutex_);
      notify =  !--count_;
    }
    if (notify)
      cv_.notify_one();
  });
}

void Worker::Join() {
  // Wait for the tasks to complete.
  std::unique_lock<std::mutex> scoped_lock(mutex_);
  cv_.wait(scoped_lock, [&]() -> bool { return !count_; });
}

}  // namespace base
