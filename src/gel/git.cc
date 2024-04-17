#include "gel/git.h"

#include <algorithm>

#include "base/log.h"

using namespace base;

Git::Git(std::vector<std::string> args)
    : args_{args}, worker_{std::thread(&Git::WorkerMain, this)} {}

Git::~Git() {
  DCHECK(!worker_.joinable())
      << "Did you forget to call TerminateWorkerThread() from derived class's "
         "destructor?";
}

void Git::TerminateWorkerThread() {
  if (worker_.joinable()) {
    quit_.store(true, std::memory_order_relaxed);
    semaphore_.release();
    worker_.join();
  }
}

// Run a new git process. Any process that was already running will be killed.
bool Git::Run(std::vector<std::string> extra_args) {
  std::vector<std::string> args = args_;
  args.insert(args.end(), extra_args.begin(), extra_args.end());
  Exec proc;
  if (!proc.Start(args))
    return false;
  DLOG(0) << "Run - pid: " << proc.pid()
          << ", status: " << static_cast<int>(proc.GetStatus());
  {
    std::lock_guard<std::mutex> scoped_lock(lock_);
    procs_[1].push_front(std::move(proc));
  }
  semaphore_.release();
  return true;
}

void Git::Kill() {
  {
    std::lock_guard<std::mutex> scoped_lock(lock_);
    procs_[1].push_front({});
  }
  semaphore_.release();
}

void Git::WorkerMain() {
  for (;;) {
    semaphore_.acquire();

    do {
      if (quit_.load(std::memory_order_relaxed))
        return;

      // Merge new processes from main thread.
      {
        std::unique_lock<std::mutex> scoped_lock(lock_, std::try_to_lock);
        if (scoped_lock)
          procs_[0].splice(procs_[0].begin(), procs_[1]);
      }

      // The latest processes we receive from the main thread is the current
      // process. Keep it running and kill the rest.
      if (!procs_[0].empty()) {
        if (curent_proc_.GetStatus() == Exec::Status::RUNNING) {
          curent_proc_.Kill();
          DLOG(0) << "Kill - pid: " << curent_proc_.pid();
          death_row_.push_back(std::move(curent_proc_));
        }
        curent_proc_ = std::move(*procs_[0].begin());
        procs_[0].pop_front();
        for (auto it = procs_[0].begin(); it != procs_[0].end(); ++it)
          it->Kill();
        death_row_.splice(death_row_.end(), procs_[0]);
        OnStart();
      }

      // Poll the current process.
      if (curent_proc_.GetStatus() == Exec::Status::RUNNING)
        Poll(curent_proc_);

      // Keep polling processes in death_row_ until they die.
      for (auto it = death_row_.begin(); it != death_row_.end();) {
        if (Poll(*it)) {
          ++it;
        } else {
          it = death_row_.erase(it);
        }
      }
    } while (curent_proc_.GetStatus() == Exec::Status::RUNNING ||
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

  if (!more && proc.pid() == curent_proc_.pid())
    OnFinished(proc.GetStatus(), proc.GetResult(), proc.GetErrStream().str());

  return more;
}
