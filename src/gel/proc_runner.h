#ifndef GEL_PROCRUNNER_H
#define GEL_PROCRUNNER_H

#include <atomic>
#include <list>
#include <memory>
#include <semaphore>
#include <string>
#include <thread>
#include <utility>

#include "base/task_runner.h"
#include "gel/exec.h"

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
  void Shutdown();

  int Run(std::vector<std::string> args);
  void Abort(int pid);

 private:
  // first: Exec ptr, second: true for running / false for aborting.
  using Proc = std::pair<std::unique_ptr<base::Exec>, bool>;

  std::thread worker_;
  std::counting_semaphore<> semaphore_{0};
  std::atomic<bool> quit_{false};
  base::TaskRunner task_runner_;

  OutputCB output_cb_;
  FinishedCB finished_cb_;
  std::shared_ptr<base::TaskRunner> main_thread_task_runner_;

  std::list<Proc> procs_;

  void WorkerMain();

  bool Poll(base::Exec* proc, const OutputCB& output_cb);

  std::list<Proc>::iterator FindProc(int pid);
};

#endif  // GEL_PROCRUNNER_H
