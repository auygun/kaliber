#ifndef GEL_GIT_DIFF_H
#define GEL_GIT_DIFF_H

#include <mutex>
#include <string>
#include <vector>

#include "gel/git.h"

class GitDiff final : public Git {
 public:
  GitDiff();
  ~GitDiff() final;

  GitDiff(GitDiff const&) = delete;
  GitDiff& operator=(GitDiff const&) = delete;

  bool Update();

  const std::vector<std::string>& GetDiffContent() const {
    return diff_content_[0];
  }

 private:
  // Double buffer for data exchange between the main thread and the worker
  // thread. [1] is accessed by both threads simultaneously. It's merged into
  // [0] in the UI thread.
  std::vector<std::string> diff_content_[2];
  bool clear_in_main_thread_ = false;
  mutable std::mutex lock_;

  void OnStarted() final;
  void OnOutput(std::string line) final;
  void OnFinished(base::Exec::Status, int result, std::string err) final;
  void OnKilled() final;
};

#endif  // GEL_GIT_DIFF_H
