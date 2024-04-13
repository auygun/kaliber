#include "gel/proc_runner.h"

#include "base/log.h"
#include "base/task_runner.h"

using namespace base;

ProcRunner::ProcRunner() = default;

ProcRunner::~ProcRunner() {
  if (worker_.joinable()) {
    quit_.store(true, std::memory_order_relaxed);
    semaphore_.release();
    worker_.join();
  }
}

void ProcRunner::Initialize(OutputCB output_cb, FinishedCB finished_cb) {
  worker_ = std::thread(&ProcRunner::WorkerMain, this);
  output_cb_ = std::move(output_cb);
  finished_cb_ = std::move(finished_cb);
  main_thread_task_runner_ = TaskRunner::GetThreadLocalTaskRunner();
  DCHECK(main_thread_task_runner_);
}

int ProcRunner::Run(std::vector<std::string> args) {
  Exec proc;
  proc.Start(args);
  int pid = proc.pid();
  DLOG(0) << "Run - pid: " << proc.pid()
          << " status: " << static_cast<int>(proc.GetStatus());
  {
    std::lock_guard<std::mutex> scoped_lock(procs_lock_);
    procs_[0].push_back(std::move(proc));
  }
  semaphore_.release();
  return pid;
}

void ProcRunner::Kill(int pid) {
  {
    std::lock_guard<std::mutex> scoped_lock(kills_lock_);
    kills_.push_back(pid);
  }
  semaphore_.release();
}

void ProcRunner::WorkerMain() {
  for (;;) {
    semaphore_.acquire();

    bool quit = quit_.load(std::memory_order_relaxed);

    do {
      {
        std::lock_guard<std::mutex> scoped_lock(procs_lock_);
        procs_[1].splice(procs_[1].end(), procs_[0]);
      }

      std::vector<int> kills;
      {
        std::lock_guard<std::mutex> scoped_lock(kills_lock_);
        kills = std::move(kills_);
        kills_.clear();
      }
      for (auto pid : kills) {
        auto it = FindProc(pid);
        if (it != procs_[1].end()) {
          DLOG(0) << "Kill - pid: " << it->pid()
                  << " status: " << static_cast<int>(it->GetStatus());
          if (it->GetStatus() == Exec::Status::RUNNING)
            it->Kill();
        }
      }

      for (auto it = procs_[1].begin(); it != procs_[1].end();) {
        if (!Poll(*it)) {
          main_thread_task_runner_->PostTask(
              HERE, std::bind(finished_cb_, it->pid(), it->GetStatus(),
                              it->GetResult(), it->GetErrStream().str()));
          it = procs_[1].erase(it);
        } else {
          ++it;
        }
      }
    } while (!procs_[1].empty());

    if (quit)
      break;
  }
}

bool ProcRunner::Poll(Exec& proc) {
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
      } else {
        main_thread_task_runner_->PostTask(
            HERE, std::bind(output_cb_, proc.pid(), std::move(line)));
      }
    }
  }

  return more;
}

std::list<Exec>::iterator ProcRunner::FindProc(int pid) {
  DCHECK(std::this_thread::get_id() == worker_.get_id());

  return std::find_if(procs_[1].begin(), procs_[1].end(),
                      [&pid](auto& a) { return a.pid() == pid; });
}
