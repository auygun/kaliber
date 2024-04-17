#include "gel/git.h"

#include <algorithm>

#include "base/log.h"

using namespace base;

Git::Git(std::vector<std::string> args)
    : args_{args}, worker_{std::thread(&Git::WorkerMain, this)} {}

Git::~Git() {
  if (worker_.joinable()) {
    quit_.store(true, std::memory_order_relaxed);
    semaphore_.release();
    worker_.join();
  }
}

bool Git::Run(std::vector<std::string> extra_args) {
  // Kill the last process and start a new one.
  if (current_pid_.load(std::memory_order_relaxed))
    Kill();

  std::vector<std::string> args;
  args.reserve(args_.size() + extra_args.size());
  args.insert(args.end(), args_.begin(), args_.end());
  args.insert(args.end(), extra_args.begin(), extra_args.end());
  Exec proc;
  if (!proc.Start(args))
    return false;
  current_pid_.store(proc.pid(), std::memory_order_relaxed);
  DLOG(0) << "Run - pid: " << proc.pid()
          << ", status: " << static_cast<int>(proc.GetStatus());
  {
    std::lock_guard<std::mutex> scoped_lock(lock_);
    procs_[1].push_back(std::move(proc));
  }
  semaphore_.release();
  return true;
}

void Git::Kill() {
  int pid = current_pid_.load(std::memory_order_relaxed);
  if (!pid)
    return;
  current_pid_.store(0, std::memory_order_relaxed);
  {
    std::lock_guard<std::mutex> scoped_lock(lock_);
    procs_to_be_killed_.push_back(pid);
  }
  semaphore_.release();
}

void Git::WorkerMain() {
  for (;;) {
    semaphore_.acquire();

    do {
      if (quit_.load(std::memory_order_relaxed))
        return;

      // Receive new processes and kill requests from main thread.
      std::vector<int> kills;
      {
        std::unique_lock<std::mutex> scoped_lock(lock_, std::try_to_lock);
        if (scoped_lock) {
          if (!procs_[1].empty())
            procs_[0].splice(procs_[0].end(), procs_[1]);
          if (!procs_to_be_killed_.empty()) {
            kills = std::move(procs_to_be_killed_);
            procs_to_be_killed_.clear();
          }
        }
      }

      // Process kill requests.
      for (auto pid : kills) {
        auto it = FindProc(pid);
        if (it != procs_[0].end()) {
          DLOG(0) << "Kill - pid: " << it->pid()
                  << ", status: " << static_cast<int>(it->GetStatus());
          if (it->GetStatus() == Exec::Status::RUNNING)
            it->Kill();
        }
      }

      // Poll all running processes.
      for (auto it = procs_[0].begin(); it != procs_[0].end();) {
        if (!Poll(*it)) {
          if (it->pid() == current_pid_.load(std::memory_order_relaxed)) {
            current_pid_.store(0, std::memory_order_relaxed);
            OnFinished(it->GetStatus(), it->GetResult(),
                       it->GetErrStream().str());
          }
          it = procs_[0].erase(it);
        } else {
          ++it;
        }
      }
    } while (!procs_[0].empty());
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
      } else if (proc.pid() == current_pid_.load(std::memory_order_relaxed)) {
        OnOutput(std::move(line));
      }
    }
  }

  return more;
}

std::list<Exec>::iterator Git::FindProc(int pid) {
  DCHECK(std::this_thread::get_id() == worker_.get_id());

  return std::find_if(procs_[0].begin(), procs_[0].end(),
                      [&pid](auto& a) { return a.pid() == pid; });
}
