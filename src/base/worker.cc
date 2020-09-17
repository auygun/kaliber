#include "worker.h"

#include "log.h"

namespace base {

Worker* Worker::singleton = nullptr;

Worker::Worker() {
  DCHECK(!singleton);
  singleton = this;
}

Worker::~Worker() {
  Shutdown();
  singleton = nullptr;
}

void Worker::Initialize(unsigned max_concurrency) {
  if (max_concurrency > std::thread::hardware_concurrency() ||
      max_concurrency == 0) {
    max_concurrency = std::thread::hardware_concurrency();
    if (max_concurrency == 0)
      max_concurrency = 1;
  }

  while (max_concurrency--)
    threads_.emplace_back(&Worker::WorkerMain, this);
}

void Worker::Shutdown() {
  if (threads_.empty())
    return;

  quit_.store(true, std::memory_order_relaxed);
  semaphore_.Release();

  for (auto& thread : threads_)
    thread.join();
  threads_.clear();
}

void Worker::PostTask(const Location& from, Closure task) {
  DCHECK((!threads_.empty()));

  task_runner_.PostTask(from, std::move(task));
  semaphore_.Release();
}

void Worker::PostTaskAndReply(const Location& from,
                              Closure task,
                              Closure reply) {
  DCHECK((!threads_.empty()));

  task_runner_.PostTaskAndReply(from, std::move(task), std::move(reply));
  semaphore_.Release();
}

void Worker::WorkerMain() {
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
