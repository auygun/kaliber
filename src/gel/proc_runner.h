#ifndef GEL_PROCRUNNER_H
#define GEL_PROCRUNNER_H

#include <atomic>
#include <functional>
#include <list>
#include <memory>
#include <mutex>
#include <semaphore>
#include <string>
#include <thread>
#include <vector>

#include "gel/exec.h"

namespace base {
class TaskRunner;
}

class ProcRunner {
 public:
  using OutputCB = std::function<void(int pid, std::string line)>;
  using FinishedCB = std::function<
      void(int pid, base::Exec::Status, int result, std::string err)>;

  ProcRunner();
  ~ProcRunner();

  ProcRunner(ProcRunner const&) = delete;
  ProcRunner& operator=(ProcRunner const&) = delete;

  void Initialize(OutputCB output_cb, FinishedCB finished_cb);

  int Run(std::vector<std::string> args);
  void Kill(int pid);

 private:
  std::thread worker_;
  std::counting_semaphore<> semaphore_{0};
  std::atomic<bool> quit_{false};

  std::list<base::Exec> procs_[2];
  std::mutex procs_lock_;

  std::vector<int> kills_;
  std::mutex kills_lock_;

  OutputCB output_cb_;
  FinishedCB finished_cb_;

  std::shared_ptr<base::TaskRunner> main_thread_task_runner_;

  void WorkerMain();

  bool Poll(base::Exec& proc);

  std::list<base::Exec>::iterator FindProc(int pid);
};

#endif  // GEL_PROCRUNNER_H