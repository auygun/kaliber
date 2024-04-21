#ifndef GEL_GEL_H
#define GEL_GEL_H

#include "engine/game.h"
#include "gel/git_diff.h"
#include "gel/git_log.h"

class Gel : public eng::Game {
 public:
  Gel();
  ~Gel() override;

  bool Initialize() override;

  void Update(float delta_time) override;

 private:
  GitLog git_log_;
  GitDiff git_diff_;

  int commit_count_ = 0;
  float diff_content_width_ = 0.0f;

  void LayoutCommitHistory(bool reset_scroll_pos);
  void LayoutCommitDiff();
};

#endif  // GEL_GEL_H
