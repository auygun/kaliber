#ifndef GEL_GIT_H
#define GEL_GIT_H

#include <atomic>
#include <list>
#include <mutex>
#include <semaphore>
#include <string>
#include <thread>
#include <vector>

#include "gel/exec.h"

class Git {
 public:
  Git(std::vector<std::string> args);
  virtual ~Git();

  Git(Git const&) = delete;
  Git& operator=(Git const&) = delete;

  bool Run(std::vector<std::string> extra_args);
  void Kill();

 protected:
  void TerminateWorkerThread();

 private:
  std::vector<std::string> args_;

  // Double buffer for data exchange between the main thread and the worker
  // thread. [1] is accessed by both threads simultaneously.
  std::list<base::Exec> procs_[2];
  std::mutex lock_;

  base::Exec curent_proc_;
  std::list<base::Exec> death_row_;

  std::counting_semaphore<> semaphore_{0};
  std::atomic<bool> quit_{false};
  std::thread worker_;

  void WorkerMain();

  bool Poll(base::Exec& proc);

  virtual void OnStart() = 0;
  virtual void OnOutput(std::string line) = 0;
  virtual void OnFinished(base::Exec::Status, int result, std::string err) = 0;
};

#endif  // GEL_GIT_H
