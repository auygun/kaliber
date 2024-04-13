#ifndef GEL_GEL_H
#define GEL_GEL_H

#include <memory>

#include "engine/game.h"
#include "gel/proc_runner.h"

class Gel : public eng::Game {
 public:
  Gel();
  ~Gel() override;

  bool Initialize() override;

  void Update(float delta_time) override;

 private:
  ProcRunner proc_runner_;

  void OnGitOutput(int pid, std::string line);
  void OnGitFinished(int pid,
                     base::Exec::Status status,
                     int result,
                     std::string err);
};

#endif  // GEL_GEL_H
