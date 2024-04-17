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

 private:
  std::vector<std::string> args_;

  std::atomic<int> current_pid_{0};
  // [0] contains the running processes polled by the worker thread. [1] is the
  // temporary buffer that keeps the new processes started in the main thread
  // which is merged into [0]. [1] is accessed by both threads simultaneously.
  std::list<base::Exec> procs_[2];
  // Accessed by both threads simultaneously.
  std::vector<int> procs_to_be_killed_;
  std::mutex lock_;

  std::counting_semaphore<> semaphore_{0};
  std::atomic<bool> quit_{false};
  std::thread worker_;

  void WorkerMain();

  bool Poll(base::Exec& proc);

  std::list<base::Exec>::iterator FindProc(int pid);

  virtual void OnOutput(std::string line) = 0;
  virtual void OnFinished(base::Exec::Status, int result, std::string err) = 0;
};

#endif  // GEL_GIT_H
