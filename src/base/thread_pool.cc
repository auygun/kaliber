#include "thread_pool.h"

#include <condition_variable>
#include <mutex>
#include <thread>
#include <vector>

#include "log.h"
#include "task_runner.h"

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

  {
    std::unique_lock<std::mutex> scoped_lock(mutex_);
    quit_when_idle_ = true;
  }
  cv_.notify_all();

  for (auto& thread : threads_)
    thread.join();
  threads_.clear();
}

void ThreadPool::EnqueueTask(Location from, Closure task) {
  DCHECK((!threads_.empty()));

  task_runner_.EnqueueTask(std::move(from), std::move(task));
  WakeUpOne();
}

void ThreadPool::EnqueueTaskAndReply(Location from,
                                     Closure task,
                                     Closure reply) {
  DCHECK((!threads_.empty()));

  task_runner_.EnqueueTaskAndReply(std::move(from), std::move(task),
                                   std::move(reply));
  WakeUpOne();
}

void ThreadPool::WakeUpOne() {
  {
    std::unique_lock<std::mutex> scoped_lock(mutex_);
    wake_up_ = true;
  }
  cv_.notify_one();
}

void ThreadPool::WorkerMain() {
  for (;;) {
    {
      std::unique_lock<std::mutex> scoped_lock(mutex_);
      while (!wake_up_) {
        if (quit_when_idle_)
          return;
        cv_.wait(scoped_lock);
      }
      wake_up_ = false;
    }

    task_runner_.MultiConsumerRun();

    DCHECK(TaskRunner::GetThreadLocalTaskRunner().IsEmpty())
        << "Pooled thread is not allowed to run tasks from the thread-local "
           "TaskRunner.";
  }
}

}  // namespace base
