#include "gel/proc_runner.h"

#include "base/log.h"

using namespace base;

ProcRunner::ProcRunner() = default;
ProcRunner::~ProcRunner() = default;

void ProcRunner::Initialize(OutputCB output_cb, FinishedCB finished_cb) {
  worker_ = std::thread(&ProcRunner::WorkerMain, this);
  output_cb_ = std::move(output_cb);
  finished_cb_ = std::move(finished_cb);
  main_thread_task_runner_ = TaskRunner::GetThreadLocalTaskRunner();
  DCHECK(main_thread_task_runner_);
}

void ProcRunner::Shutdown() {
  quit_.store(true, std::memory_order_relaxed);
  semaphore_.release();
  worker_.join();
}

int ProcRunner::Run(std::vector<std::string> args) {
  auto proc = new Exec();
  proc->Start(args);
  int pid = proc->pid();
  DLOG(0) << "Run - pid: " << proc->pid()
          << " status: " << static_cast<int>(proc->GetStatus());

  task_runner_.PostTask(HERE, [&, proc]() -> void {
    DCHECK(FindProc(proc->pid()) == procs_.end());
    procs_.emplace_back(proc);
  });
  semaphore_.release();

  return pid;
}

void ProcRunner::Kill(int pid) {
  task_runner_.PostTask(HERE, [&, pid]() -> void {
    auto it = FindProc(pid);
    if (it == procs_.end())
      return;

    DLOG(0) << "Kill - pid: " << (*it)->pid()
            << " status: " << static_cast<int>((*it)->GetStatus());
    if ((*it)->GetStatus() == Exec::Status::RUNNING)
      (*it)->Kill();
  });
  semaphore_.release();
}

void ProcRunner::WorkerMain() {
  for (;;) {
    semaphore_.acquire();

    if (quit_.load(std::memory_order_relaxed))
      break;

    do {
      task_runner_.RunTasks();

      for (auto it = procs_.begin(); it != procs_.end();) {
        if (!Poll((*it).get())) {
          main_thread_task_runner_->PostTask(
              HERE, std::bind(finished_cb_, (*it)->pid(), (*it)->GetStatus(),
                              (*it)->GetResult(), (*it)->GetErrStream().str()));
          it = procs_.erase(it);
        } else {
          ++it;
        }
      }
    } while (!procs_.empty());
  }
}

bool ProcRunner::Poll(Exec* proc) {
  DCHECK(std::this_thread::get_id() == worker_.get_id());

  bool more = proc->Poll();

  while (!proc->GetOutStream().eof()) {
    auto last_pos = proc->GetOutStream().tellg();
    std::string line;
    std::getline(proc->GetOutStream(), line);

    if (!(proc->GetOutStream().rdstate() & std::ios_base::failbit)) {
      if (more && proc->GetOutStream().eof()) {
        // Incomplete line. Rewind and wait for more data.
        proc->GetOutStream().seekg(last_pos);
        proc->GetOutStream().setstate(std::ios_base::eofbit);
      } else {
        main_thread_task_runner_->PostTask(
            HERE, std::bind(output_cb_, proc->pid(), std::move(line)));
      }
    }
  }

  return more;
}

std::list<ProcRunner::Proc>::iterator ProcRunner::FindProc(int pid) {
  DCHECK(std::this_thread::get_id() == worker_.get_id());

  return std::find_if(procs_.begin(), procs_.end(),
                      [&pid](auto& a) { return a->pid() == pid; });
}
