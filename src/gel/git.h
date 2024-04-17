#ifndef GEL_GIT_H
#define GEL_GIT_H

#include <atomic>
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

  void Update();

  void RefreshCommitHistory();

  const std::vector<CommitInfo>& GetCommitHistory() const {
    return commit_history_[0];
  }

 private:
  CommitInfo current_commit_;
  // [0] is the commit history accessed by the UI thread. [1] is the temporary
  // buffer to accumulate commits in the worker thread which is merged into [0].
  std::vector<CommitInfo> commit_history_[2];
  mutable std::mutex commit_history_lock_;
  std::atomic<size_t> commit_history_size_{0};

  ProcRunner git_cmd_log_;

  void OnGitOutput(int pid, std::string line);
  void OnGitFinished(int pid,
                     base::Exec::Status status,
                     int result,
                     std::string err);

  void PushCurrentCommitToBuffer();
};

#endif  // GEL_GIT_H
