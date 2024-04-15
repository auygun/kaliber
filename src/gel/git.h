#ifndef GEL_GIT_H
#define GEL_GIT_H

#include <mutex>
#include <string>
#include <vector>

#include "gel/proc_runner.h"

// Backend that runs git commands and parses the output. Collects the result and
// passes to frontend on demand.
class Git {
 public:
  struct CommitInfo {
    std::string commit;
    std::vector<std::string> parents;
    std::string author;
    std::string author_date;
    std::string committer;
    std::string committer_date;
  };

  Git();
  ~Git();

  Git(Git const&) = delete;
  Git& operator=(Git const&) = delete;

  void RefreshCommitHistory();

  // Access to the commit history
  size_t GetCommitHistorySize() const;
  CommitInfo GetCommit(size_t index) const;

 private:
  CommitInfo current_commit_;
  std::vector<CommitInfo> commit_history_;
  mutable std::mutex commit_history_lock_;

  ProcRunner git_cmd_log_;

  void OnGitOutput(int pid, std::string line);
  void OnGitFinished(int pid,
                     base::Exec::Status status,
                     int result,
                     std::string err);
};

#endif  // GEL_GIT_H
