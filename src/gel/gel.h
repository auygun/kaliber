#ifndef GEL_GEL_H
#define GEL_GEL_H

#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "engine/game.h"
#include "gel/proc_runner.h"

struct CommitInfo {
  std::string commit;
  std::vector<std::string> parents;
  std::string author;
  std::string author_date;
  std::string committer;
  std::string committer_date;
};

class Gel : public eng::Game {
 public:
  Gel();
  ~Gel() override;

  bool Initialize() override;

  void Update(float delta_time) override;

 private:
  CommitInfo current_commit_;
  std::vector<CommitInfo> commit_history_;
  std::mutex commit_history_lock_;

  ProcRunner proc_runner_;

  void OnGitOutput(int pid, std::string line);
  void OnGitFinished(int pid,
                     base::Exec::Status status,
                     int result,
                     std::string err);
};

#endif  // GEL_GEL_H
