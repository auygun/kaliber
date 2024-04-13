#include "gel/exec.h"

#include <poll.h>
#include <sys/wait.h>

#include "base/log.h"

namespace base {

bool Exec::Start(const std::vector<std::string>& args) {
  DCHECK(status_ == Status::UNINITIALIZED);

  if (!out_pipe_.Create()) {
    LOG(0) << "out_pipe failed";
    status_ = Status::SYSTEM_ERROR;
    return false;
  }
  if (!err_pipe_.Create()) {
    LOG(0) << "err_pipe failed";
    status_ = Status::SYSTEM_ERROR;
    return false;
  }

  pid_ = fork();
  if (pid_ < 0) {
    LOG(0) << "fork() failed";
    status_ = Status::SYSTEM_ERROR;
    return false;
  }
  if (pid_ == 0) {
    // Child
    close(STDIN_FILENO);
    if (dup2(out_pipe_[Pipe::kWrite], STDOUT_FILENO) < 0 ||
        dup2(err_pipe_[Pipe::kWrite], STDERR_FILENO) < 0)
      _exit(-1);
    out_pipe_.CloseAll();
    err_pipe_.CloseAll();

    std::vector<const char*> pargs;
    pargs.reserve(args.size());
    for (auto const& arg : args) {
      pargs.push_back(arg.c_str());
    }
    pargs.push_back(nullptr);
    execvp(pargs.front(), const_cast<char* const*>(pargs.data()));
    _exit(-1);
  } else {
    status_ = Status::RUNNING;
    out_pipe_.Close(Pipe::kWrite);
    out_pipe_.MakeNonBlocking(Pipe::kRead);
    err_pipe_.Close(Pipe::kWrite);
    err_pipe_.MakeNonBlocking(Pipe::kRead);
  }
  return true;
}

bool Exec::Poll() {
  DCHECK(status_ != Status::UNINITIALIZED);
  DCHECK(status_ != Status::SYSTEM_ERROR);

  struct pollfd fd[2];
  size_t fds = 0;
  char buffer[32768];

  if (out_pipe_) {
    fd[fds].fd = out_pipe_[Pipe::kRead];
    fd[fds].events = POLL_IN;
    ++fds;
  }
  if (err_pipe_) {
    fd[fds].fd = err_pipe_[Pipe::kRead];
    fd[fds].events = POLL_IN;
    ++fds;
  }
  if (fds > 0 && poll(fd, fds, 1) < 0) {
    LOG(0) << "poll failed";
    status_ = Status::SYSTEM_ERROR;
    return false;
  }

  if (out_pipe_) {
    auto bytes_read = read(out_pipe_[Pipe::kRead], buffer, sizeof(buffer));
    if (bytes_read == 0) {
      out_pipe_.Close(Pipe::kRead);
    } else if (bytes_read > 0) {
      out_.clear();
      out_.write(buffer, bytes_read);
    } else if (errno != EAGAIN && errno != EWOULDBLOCK) {
      LOG(0) << "read out_pipe failed";
      status_ = Status::SYSTEM_ERROR;
      return false;
    }
  }

  if (err_pipe_) {
    auto bytes_read = read(err_pipe_[Pipe::kRead], buffer, sizeof(buffer));
    if (bytes_read == 0) {
      err_pipe_.Close(Pipe::kRead);
    } else if (bytes_read > 0) {
      err_.clear();
      err_.write(buffer, bytes_read);
    } else if (errno != EAGAIN && errno != EWOULDBLOCK) {
      LOG(0) << "read err_pipe failed";
      status_ = Status::SYSTEM_ERROR;
      return false;
    }
  }

  if (status_ == Status::RUNNING) {
    int proc_status = 0;
    pid_t ret = waitpid(pid_, &proc_status, WNOHANG);
    if (ret < 0) {
      LOG(0) << "waitpid failed";
      status_ = Status::SYSTEM_ERROR;
      return false;
    }
    if (ret != 0) {
      if (WIFEXITED(proc_status)) {
        status_ = Status::EXITED;
        result_ = WEXITSTATUS(proc_status);
      } else if (WIFSIGNALED(proc_status) && WTERMSIG(proc_status) == SIGINT) {
        status_ = Status::KILLED;
      } else {
        LOG(0) << "exec_status: " << proc_status;
        status_ = Status::SYSTEM_ERROR;
        return false;
      }
    }
  }

  return out_pipe_ || err_pipe_ || status_ == Status::RUNNING;
}

bool Exec::Kill() {
  DCHECK(status_ == Status::RUNNING);
  if (kill(pid_, SIGINT)) {
    LOG(0) << "kill failed";
    status_ = Status::SYSTEM_ERROR;
    return false;
  }
  return true;
}

}  // namespace base
