#include "gel/git.h"

#include <functional>
#include <iostream>

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

void Git::Update() {
  // Merge buffered commits into commit_history_[0] if lock succeeds. Otherwise,
  // we will keep buffering and try again later.
  std::unique_lock<std::mutex> scoped_lock(commit_history_lock_,
                                           std::try_to_lock);
  if (scoped_lock && commit_history_[1].size() > 0) {
    commit_history_[0].insert(commit_history_[0].end(),
                              commit_history_[1].begin(),
                              commit_history_[1].end());
    commit_history_[1].clear();
  }
}

void Git::RefreshCommitHistory() {
  git_cmd_log_.Run(
      {"git", "log", "--color=never", "--parents", "--pretty=fuller", "-z"});
}

void Git::OnGitOutput(int pid, std::string line) {
  if (line.empty())
    return;

  // Commits are separated with nulls.
  if (line.data()[0] == 0) {
    PushCurrentCommitToBuffer();
    line = line.substr(1);  // Skip the null character.
  }

  std::istringstream iss(line);
  std::string field, value;
  std::getline(iss, field, ' ');
  std::getline(iss, value);

  using namespace std::string_literals;
  if ("commit"s == field)
    current_commit_.commit = value;
  else if ("Author:"s == field)
    current_commit_.author = value;
  else if ("AuthorDate:"s == field)
    current_commit_.author_date = value;
}

void Git::OnGitFinished(int pid,
                        Exec::Status status,
                        int result,
                        std::string err) {
  LOG(0) << "Finished pid: " << pid << " status: " << static_cast<int>(status)
         << " result: " << result << " err: " << err;
  PushCurrentCommitToBuffer();
}

void Git::PushCurrentCommitToBuffer() {
  {
    std::lock_guard<std::mutex> scoped_lock(commit_history_lock_);
    commit_history_[1].push_back(current_commit_);
  }
  current_commit_ = {};
}
