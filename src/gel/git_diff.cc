#include "gel/git_diff.h"

#include "base/log.h"

using namespace base;

GitDiff::GitDiff()
    : Git{{"git", "diff-tree", "-r", "-p", "--textconv", "--submodule", +"-C",
           "--cc", "--no-commit-id", "-U3", "--root"}} {}

GitDiff::~GitDiff() {
  TerminateWorkerThread();
}

void GitDiff::Update() {
  // Merge buffered data if lock succeeds. Otherwise, we will keep buffering and
  // try again later.
  std::unique_lock<std::mutex> scoped_lock(lock_, std::try_to_lock);
  if (scoped_lock) {
    if (clear_in_main_thread_) {
      clear_in_main_thread_ = false;
      diff_content_[0].clear();
    }
    if (diff_content_[1].size() > 0) {
      diff_content_[0].insert(diff_content_[0].end(), diff_content_[1].begin(),
                              diff_content_[1].end());
      diff_content_[1].clear();
    }
  }
}

void GitDiff::OnStarted() {
  std::lock_guard<std::mutex> scoped_lock(lock_);
  diff_content_[1].clear();
  clear_in_main_thread_ = true;
}

void GitDiff::OnOutput(std::string line) {
  if (line.empty())
    return;

  {
    std::lock_guard<std::mutex> scoped_lock(lock_);
    diff_content_[1].push_back(line);
  }
}

void GitDiff::OnKilled() {}

void GitDiff::OnFinished(Exec::Status status, int result, std::string err) {}
