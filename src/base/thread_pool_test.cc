// Repro for a deadlock in ThreadPool's sequenced task runners.
//
// Scenario (2 workers):
//   1. Create a sequenced task runner via ThreadPool::CreateSequencedTaskRunner().
//   2. Post two tasks, T1 and T2, to it. Each post releases the pool semaphore,
//      so both workers wake up.
//   3. Worker A wins the runner's processing_lock (try_lock) and starts its
//      batch. T1 posts T3 to the same runner (the "next batch"). The post
//      releases the pool semaphore, which wakes worker B.
//   4. Worker B try-locks the runner's processing_lock, but fails: worker A is
//      still running its batch (T2 sleeps to make this deterministic). Worker B
//      finds no other work and goes back to sleep -- having consumed the
//      semaphore release that T3's post produced.
//   5. Worker A finishes its batch (T3 was posted *after* it swapped the
//      queue, so it is not in A's batch), releases the lock and goes back to
//      sleep. T3 is left in the queue with no semaphore release left to wake
//      anyone: no worker will ever pick up the next batch.
//
// Expected (buggy) result: T3 never runs, GetPendingTaskCount() stays at 1.
// Fixed result: T3 runs, no deadlock.
//
// Build (from the repo root):
//   c++ -std=c++20 -O2 -pthread -Isrc -DNDEBUG \
//       src/base/thread_pool_test.cc src/base/thread_pool.cc \
//       src/base/task_runner.cc src/base/log.cc -o thread_pool_test

#include <atomic>
#include <chrono>
#include <cstdio>

#include "base/thread_pool.h"

namespace {

using namespace std::chrono_literals;

constexpr auto kLockHoldTime = 1s;   // How long T2 keeps worker A busy.
constexpr auto kTimeout = 3s;        // How long to wait for T3 to run.

}  // namespace

int main() {
  base::ThreadPool pool;
  pool.Initialize(/*max_concurrency=*/2);

  auto seq = pool.CreateSequencedTaskRunner();

  std::atomic<bool> t3_ran{false};

  // T1: the first task. While running (on worker A, which holds the runner's
  // processing_lock), it posts T3 to the same sequenced runner.
  seq->PostTask(HERE, [&seq, &t3_ran]() {
    seq->PostTask(HERE, [&t3_ran]() { t3_ran.store(true); });
  });

  // T2: keeps worker A running (and holding the processing_lock) long enough
  // that worker B's try-lock is guaranteed to fail.
  seq->PostTask(HERE, []() { std::this_thread::sleep_for(kLockHoldTime); });

  auto start = std::chrono::steady_clock::now();
  while (!t3_ran.load() && std::chrono::steady_clock::now() - start < kTimeout)
    std::this_thread::sleep_for(1ms);

  if (t3_ran.load()) {
    std::printf("PASS: T3 ran, no deadlock.\n");
    return 0;
  }

  std::printf("FAIL: deadlock reproduced.\n");
  std::printf("  T3 never ran; pending task count: %zu\n",
              pool.GetPendingTaskCount());
  return 1;
}
