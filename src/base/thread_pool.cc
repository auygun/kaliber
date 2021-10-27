#include "base/thread_pool.h"

#include "base/log.h"

namespace base {

ThreadPool* ThreadPool::singleton = nullptr;

ThreadPool::ThreadPool() {
  DCHECK(!singleton);
  singleton = this;
}

ThreadPool::~ThreadPool() {
  Shutdown();
  singleton = nullptr;
}

void ThreadPool::Initialize(unsigned max_concurrency) {
  if (max_concurrency > std::thread::hardware_concurrency() ||
      max_concurrency == 0) {
    max_concurrency = std::thread::hardware_concurrency();
    if (max_concurrency == 0)
      max_concurrency = 1;
  }

  while (max_concurrency--)
    threads_.emplace_back(&ThreadPool::WorkerMain, this);
}

void ThreadPool::Shutdown() {
  if (threads_.empty())
    return;

  quit_.store(true, std::memory_order_relaxed);
  semaphore_.Release();

  for (auto& thread : threads_)
    thread.join();
  threads_.clear();
}

void ThreadPool::PostTask(const Location& from, Closure task) {
  DCHECK((!threads_.empty()));

  task_runner_.PostTask(from, std::move(task));
  semaphore_.Release();
}

void ThreadPool::PostTaskAndReply(const Location& from,
                                  Closure task,
                                  Closure reply) {
  DCHECK((!threads_.empty()));

  task_runner_.PostTaskAndReply(from, std::move(task), std::move(reply));
  semaphore_.Release();
}

void ThreadPool::WorkerMain() {
  for (;;) {
    semaphore_.Acquire();

    if (quit_.load(std::memory_order_relaxed)) {
      semaphore_.Release();
      return;
    }

    task_runner_.MultiConsumerRun();
  }
}

}  // namespace base
