#include "gel/git_log.h"

#include <optional>

#include "base/log.h"

using namespace base;

namespace {

std::optional<std::string> GetStringAfterLabel(const std::string& str,
                                               const std::string& label) {
  if (str.substr(0, label.length()) != label)
    return std::nullopt;
  size_t start = label.length();
  while (start < str.length() && std::isspace(str[start]))
    start++;
  return std::optional<std::string>{str.substr(start)};
}

}  // namespace

GitLog::GitLog()
    : Git{{"git", "log", "--color=never", /*"--parents",*/ "--pretty=fuller",
           "-z"}} {}

GitLog::~GitLog() {
  TerminateWorkerThread();
}

void GitLog::Update() {
  // Merge buffered data if lock succeeds. Otherwise, we will keep buffering and
  // try again later.
  std::unique_lock<std::mutex> scoped_lock(lock_, std::try_to_lock);
  if (scoped_lock) {
    if (clear_in_main_thread_) {
      clear_in_main_thread_ = false;
      commit_history_[0].clear();
    }
    if (commit_history_[1].size() > 0) {
      commit_history_[0].insert(commit_history_[0].end(),
                                commit_history_[1].begin(),
                                commit_history_[1].end());
      commit_history_[1].clear();
    }
  }
}

void GitLog::OnStarted() {
  {
    std::lock_guard<std::mutex> scoped_lock(lock_);
    commit_history_[1].clear();
    clear_in_main_thread_ = true;
  }
  current_commit_ = {};
}

void GitLog::OnOutput(std::string line) {
  if (line.empty())
    return;

  // Commits are separated with nulls.
  if (line.data()[0] == 0) {
    PushCurrentCommitToBuffer();
    line = line.substr(1);  // Skip the null character.
  }

  // LOG(0) << line;

  if (auto val = GetStringAfterLabel(line, "    ")) {
    current_commit_.message.push_back(*val);
  } else if (auto val = GetStringAfterLabel(line, "commit")) {
    current_commit_.commit = *val;
  } else if (auto val = GetStringAfterLabel(line, "Author:")) {
    current_commit_.author = *val;
  } else if (auto val = GetStringAfterLabel(line, "AuthorDate:")) {
    current_commit_.author_date = *val;
  } else if (auto val = GetStringAfterLabel(line, "Commit:")) {
    current_commit_.committer = *val;
  } else if (auto val = GetStringAfterLabel(line, "CommitDate:")) {
    current_commit_.committer_date = *val;
  }
}

void GitLog::OnFinished(Exec::Status status, int result, std::string err) {
  PushCurrentCommitToBuffer();
}

void GitLog::OnKilled() {}

void GitLog::PushCurrentCommitToBuffer() {
  if (current_commit_.message.empty())
    current_commit_.message.push_back("");
  {
    std::lock_guard<std::mutex> scoped_lock(lock_);
    commit_history_[1].push_back(current_commit_);
  }
  current_commit_ = {};
}
