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

  TaskRunner::GetLocalTaskRunner().SetDelegate(this);
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
  task_runner_.EnqueueTask(std::move(from), std::move(task));
  cv_.notify_one();
}

void ThreadPool::EnqueueTaskAndReply(Location from,
                                     Closure task,
                                     Closure reply) {
  task_runner_.EnqueueTaskAndReply(std::move(from), std::move(task),
                                   std::move(reply));
  cv_.notify_one();
}

void ThreadPool::Signal() {
  cv_.notify_all();
}

void ThreadPool::WorkerMain() {
  for (;;) {
    {
      std::unique_lock<std::mutex> scoped_lock(mutex_);
      while (task_runner_.Enmpty() &&
             TaskRunner::GetLocalTaskRunner().Enmpty()) {
        if (quit_when_idle_)
          return;
        cv_.wait(scoped_lock);
      }
    }

    task_runner_.Run();

    TaskRunner::GetLocalTaskRunner().Run();
  }
}

}  // namespace base
