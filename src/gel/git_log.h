#ifndef GEL_GIT_LOG_H
#define GEL_GIT_LOG_H

#include <mutex>
#include <string>
#include <vector>

#include "gel/git.h"

class GitLog final : public Git {
 public:
  struct CommitInfo {
    std::string commit;
    std::vector<std::string> parents;
    std::string author;
    std::string author_date;
    std::string committer;
    std::string committer_date;
  };

  GitLog();
  ~GitLog() final;

  GitLog(GitLog const&) = delete;
  GitLog& operator=(GitLog const&) = delete;

  void Update();

  const std::vector<CommitInfo>& GetCommitHistory() const {
    return commit_history_[0];
  }

 private:
  CommitInfo current_commit_;
  // Double buffer for data exchange between the main thread and the worker
  // thread. [1] is accessed by both threads simultaneously. It's merged into
  // [0] in the UI thread.
  std::vector<CommitInfo> commit_history_[2];
  bool clear_in_main_thread_ = false;
  mutable std::mutex lock_;

  void OnStart() final;
  void OnOutput(std::string line) final;
  void OnFinished(base::Exec::Status, int result, std::string err) final;

  void PushCurrentCommitToBuffer();
};

#endif  // GEL_GIT_LOG_H
