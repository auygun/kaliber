#ifndef BASE_EXEC_H
#define BASE_EXEC_H

#include <sstream>
#include <string>
#include <vector>

#include "gel/pipe.h"

namespace base {

class Exec {
 public:
  enum class Status { UNINITIALIZED, RUNNING, EXITED, KILLED, SYSTEM_ERROR };

  Exec() = default;
  ~Exec() = default;

  Exec(Exec&& other) = default;
  Exec& operator=(Exec&& other) = default;

  bool Start(const std::vector<std::string>& args);

  bool Poll();

  bool Kill();

  Status GetStatus() const { return status_; }

  int GetResult() const { return result_; }

  std::stringstream& GetOutStream() { return out_; }
  std::stringstream& GetErrStream() { return err_; }

  int pid() const { return pid_; }

 private:
  Status status_ = Status::UNINITIALIZED;
  int result_ = 0;
  pid_t pid_ = 0;
  Pipe out_pipe_;
  Pipe err_pipe_;
  std::stringstream out_;
  std::stringstream err_;
};

}  // namespace base

#endif  // BASE_EXEC_H
