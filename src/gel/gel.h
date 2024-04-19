#ifndef GEL_GEL_H
#define GEL_GEL_H

#include "engine/game.h"
#include "gel/git_log.h"
#include "gel/git_diff.h"

class Gel : public eng::Game {
 public:
  Gel();
  ~Gel() override;

  bool Initialize() override;

  void Update(float delta_time) override;

 private:
  GitLog git_log_;
  GitDiff git_diff_;

  int item_counts_ = 0;
};

#endif  // GEL_GEL_H
