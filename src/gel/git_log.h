#ifndef GEL_GIT_LOG_H
#define GEL_GIT_LOG_H

#include <mutex>
#include <string>
#include <vector>

#include "gel/git.h"

// Backend that runs git commands and parses the output. Collects the result and
// passes to frontend on demand.
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

  // Updates commit history from worker thread. Call once every frame before
  // updating UI.
  void Update();

  const std::vector<CommitInfo>& GetCommitHistory() const {
    return commit_history_[0];
  }

 private:
  CommitInfo current_commit_;
  // [0] is the commit history accessed by the UI thread. [1] is the temporary
  // buffer to accumulate commits in the worker thread which is merged into [0].
  std::vector<CommitInfo> commit_history_[2];
  bool clear_history_in_main_thread_ = false;
  mutable std::mutex lock_;

  void OnStart() final;
  void OnOutput(std::string line) final;
  void OnFinished(base::Exec::Status, int result, std::string err) final;

  void PushCurrentCommitToBuffer();
};

#endif  // GEL_GIT_LOG_H
