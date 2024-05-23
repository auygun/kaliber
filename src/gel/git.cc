#include "gel/git.h"

#include <algorithm>

#include "base/log.h"

using namespace base;

Git::Git(std::vector<std::string> args)
    : args_{args}, worker_{std::thread(&Git::WorkerMain, this)} {}

Git::~Git() {
  DCHECK(!worker_.joinable())
      << "Did you forget to call TerminateWorkerThread() from derived class?";
}

void Git::TerminateWorkerThread() {
  if (worker_.joinable()) {
    quit_.store(true, std::memory_order_relaxed);
    semaphore_.release();
    worker_.join();
  }
}

bool Git::Run(std::vector<std::string> extra_args) {
  // Start a new git process and pass it to the worker thread. The currently
  // running process will be killed and replaced by the new one.
  std::vector<std::string> args = args_;
  args.insert(args.end(), extra_args.begin(), extra_args.end());
  Exec proc;
  if (!proc.Start(args))
    return false;
  {
    std::lock_guard<std::mutex> scoped_lock(lock_);
    new_procs_.push_front(std::move(proc));
  }
  semaphore_.release();
  return true;
}

void Git::Kill() {
  // Pass an uninitialized Exec the the worker thread. This will cause the
  // currently running process to be killed.
  {
    std::lock_guard<std::mutex> scoped_lock(lock_);
    new_procs_.push_front({});
  }
  semaphore_.release();
}

void Git::WorkerMain() {
  for (;;) {
    semaphore_.acquire();

    do {
      if (quit_.load(std::memory_order_relaxed))
        return;

      // Get new processes from main thread.
      std::list<base::Exec> procs;
      {
        std::lock_guard<std::mutex> scoped_lock(lock_);
        procs.swap(new_procs_);
      }

      if (!procs.empty()) {
        // Kill the old process and keep it in death_row_.
        if (curent_proc_.GetStatus() == Exec::Status::RUNNING) {
          curent_proc_.Kill();
          OnKilled();
          DLOG(0) << "Killed - pid: " << curent_proc_.pid();
          death_row_.push_back(std::move(curent_proc_));
        }

        // Replace the current process with the latest processes we received
        // from the main thread.
        curent_proc_ = std::move(*procs.begin());
        procs.pop_front();
        if (curent_proc_.GetStatus() == Exec::Status::RUNNING) {
          OnStarted();
          DLOG(0) << "Started - pid: " << curent_proc_.pid();
        }

        // Kill any remaining process that was started before the last one and
        // keep then in death_row_.
        if (!procs.empty()) {
          for (auto& proc : procs) {
            if (proc.GetStatus() == Exec::Status::RUNNING) {
              proc.Kill();
              OnKilled();
              DLOG(0) << "Killed - pid: " << proc.pid();
            }
          }
          death_row_.splice(death_row_.end(), procs);
        }
      }

      // Poll the current process.
      if (curent_proc_.GetStatus() != Exec::Status::UNINITIALIZED &&
          !Poll(curent_proc_)) {
        DLOG(0) << "Terminated - pid: " << curent_proc_.pid();
        curent_proc_ = {};
      }

      // Keep polling all killed processes until they are terminated.
      for (auto it = death_row_.begin(); it != death_row_.end();) {
        if (curent_proc_.GetStatus() != Exec::Status::UNINITIALIZED &&
            Poll(*it)) {
          ++it;
        } else {
          DLOG(0) << "Terminated (was killed) - pid: " << it->pid();
          it = death_row_.erase(it);
        }
      }
    } while (curent_proc_.GetStatus() != Exec::Status::UNINITIALIZED ||
             !death_row_.empty());
  }
}

bool Git::Poll(Exec& proc) {
  DCHECK(std::this_thread::get_id() == worker_.get_id());

  bool more = proc.Poll();

  while (!proc.GetOutStream().eof()) {
    auto last_pos = proc.GetOutStream().tellg();
    std::string line;
    std::getline(proc.GetOutStream(), line);

    if (!(proc.GetOutStream().rdstate() & std::ios_base::failbit)) {
      if (more && proc.GetOutStream().eof()) {
        // Incomplete line. Rewind and wait for more data.
        proc.GetOutStream().seekg(last_pos);
        proc.GetOutStream().setstate(std::ios_base::eofbit);
      } else if (proc.pid() == curent_proc_.pid()) {
        OnOutput(std::move(line));
      }
    }
  }

  if (!more && proc.pid() == curent_proc_.pid()) {
    DLOG(0) << "Finished -  pid: " << curent_proc_.pid()
            << ", status: " << static_cast<int>(curent_proc_.GetStatus())
            << ", result: " << curent_proc_.GetResult()
            << ", err: " << curent_proc_.GetErrStream().str();
    OnFinished(proc.GetStatus(), proc.GetResult(), proc.GetErrStream().str());
  }

  return more;
}
