#include "gel/git_log.h"

#include "base/log.h"

using namespace base;

GitLog::GitLog()
    : Git{{"git", "log", "--color=never", "--parents", "--pretty=fuller",
           "-z"}} {}

GitLog::~GitLog() {
  TerminateWorkerThread();
}

void GitLog::Update() {
  // Merge buffered commits into commit_history_[0] if lock succeeds. Otherwise,
  // we will keep buffering and try again later.
  std::unique_lock<std::mutex> scoped_lock(lock_, std::try_to_lock);
  if (scoped_lock && commit_history_[1].size() > 0) {
    commit_history_[0].insert(commit_history_[0].end(),
                              commit_history_[1].begin(),
                              commit_history_[1].end());
    commit_history_[1].clear();
  }
}

void GitLog::OnOutput(std::string line) {
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

void GitLog::OnFinished(Exec::Status status, int result, std::string err) {
  LOG(0) << "Finished status: " << static_cast<int>(status)
         << " result: " << result << " err: " << err;
  PushCurrentCommitToBuffer();
}

void GitLog::PushCurrentCommitToBuffer() {
  {
    std::lock_guard<std::mutex> scoped_lock(lock_);
    commit_history_[1].push_back(current_commit_);
  }
  current_commit_ = {};
}