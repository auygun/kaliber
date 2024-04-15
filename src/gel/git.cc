#include "gel/git.h"

#include <functional>
#include <regex>

#include "base/log.h"

using namespace base;

Git::Git()
    : git_cmd_log_{std::bind(&Git::OnGitOutput,
                             this,
                             std::placeholders::_1,
                             std::placeholders::_2),
                   std::bind(&Git::OnGitFinished,
                             this,
                             std::placeholders::_1,
                             std::placeholders::_2,
                             std::placeholders::_3,
                             std::placeholders::_4)} {}

Git::~Git() = default;

void Git::RefreshCommitHistory() {
  git_cmd_log_.Run(
      {"git", "log", "--color=never", "--parents", "--pretty=fuller", "-z"});
}

size_t Git::GetCommitHistorySize() const {
  size_t size;
  {
    std::lock_guard<std::mutex> scoped_lock(commit_history_lock_);
    size = commit_history_.size();
  }
  return size;
}

Git::CommitInfo Git::GetCommit(size_t index) const {
  CommitInfo commit;
  {
    std::lock_guard<std::mutex> scoped_lock(commit_history_lock_);
    DCHECK(index < commit_history_.size());
    commit = commit_history_[index];
  }
  return commit;
}

void Git::OnGitOutput(int pid, std::string line) {
  if (!line.empty() && line.data()[0] == 0) {
    {
      std::lock_guard<std::mutex> scoped_lock(commit_history_lock_);
      commit_history_.push_back(current_commit_);
    }
    current_commit_ = {};
    line = line.substr(1);
  }

  std::regex pattern("^commit\\s(\\w+)(?:\\s(\\w+))*");
  std::smatch match;

  if (std::regex_search(line, match, pattern)) {
    for (size_t i = 1; i < match.size(); ++i) {
      if (match[i].matched) {
        if (i == 1)
          current_commit_.commit = match[i];
        else
          current_commit_.parents.push_back(match[i]);
      }
    }
  } else {
    std::regex pattern("^Author:\\s*(.*)$");
    std::smatch match;

    if (std::regex_search(line, match, pattern)) {
      current_commit_.author = match[1];
    } else {
      std::regex pattern("^AuthorDate:\\s*(.*)$");
      std::smatch match;

      if (std::regex_search(line, match, pattern)) {
        current_commit_.author_date = match[1];
      } else {
      }
    }
  }
}

void Git::OnGitFinished(int pid,
                        Exec::Status status,
                        int result,
                        std::string err) {
  LOG(0) << "Finished pid: " << pid << " status: " << static_cast<int>(status)
         << " result: " << result << " err: " << err;
  {
    std::lock_guard<std::mutex> scoped_lock(commit_history_lock_);
    commit_history_.push_back(current_commit_);
  }
  current_commit_ = {};
}
