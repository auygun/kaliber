#include "thread_pool.h"

#include <thread>
#include <vector>

#include "log.h"
#include "task_runner.h"

namespace {

class ThreadPoolImpl {
 public:
  ThreadPoolImpl() = default;

  ~ThreadPoolImpl() { Shutdown(); }

  void Initialize(unsigned max_concurrency) {
    if (max_concurrency > std::thread::hardware_concurrency() ||
        max_concurrency == 0) {
      max_concurrency = std::thread::hardware_concurrency();
      if (max_concurrency == 0)
        max_concurrency = 1;
    }

    while (max_concurrency--)
      threads_.emplace_back(&ThreadPoolImpl::WorkerMain, this);
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

ThreadPoolImpl g_thread_pool;

}  // namespace

namespace base {

void ThreadPool::Initialize(unsigned max_concurrency) {
  g_thread_pool.Initialize(max_concurrency);
}

void ThreadPool::Shutdown() {
  g_thread_pool.Shutdown();
}

TaskRunner& ThreadPool::GetTaskRunner() {
  return g_thread_pool.GetTaskRunner();
}

}  // namespace base
