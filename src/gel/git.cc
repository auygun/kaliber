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
  // Lock free
  return commit_history_size_.load(std::memory_order_relaxed);
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

std::vector<Git::CommitInfo> Git::GetCommitRange(size_t start_index,
                                                 size_t end_index) const {
  std::vector<Git::CommitInfo> commits;
  {
    std::lock_guard<std::mutex> scoped_lock(commit_history_lock_);
    DCHECK(start_index < commit_history_.size());
    DCHECK(end_index - 1 < commit_history_.size());
    commits.assign(commit_history_.begin() + start_index,
                   commit_history_.begin() + end_index);
  }
  return commits;
}

void Git::OnGitOutput(int pid, std::string line) {
  if (!line.empty() && line.data()[0] == 0) {
    // Push the current commit to the buffer and start parsing a new one.
    commit_buffer_.push_back(current_commit_);
    current_commit_ = {};
    line = line.substr(1);  // Skip the null character.
    TryPushBufferToCommitHistory();
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
  if (!commit_buffer_.empty())
    TryPushBufferToCommitHistory();
}

void Git::TryPushBufferToCommitHistory() {
  // Push buffered commits to commit_history_ if lock succeeds. Keep buffering
  // and try again later otherwise.
  {
    std::unique_lock<std::mutex> scoped_lock(commit_history_lock_,
                                             std::try_to_lock);
    if (!scoped_lock)
      return;
    commit_history_.insert(commit_history_.end(), commit_buffer_.begin(),
                           commit_buffer_.end());
  }
  commit_history_size_.fetch_add(commit_buffer_.size(),
                                 std::memory_order_relaxed);
  commit_buffer_.clear();
}