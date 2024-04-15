#ifndef GEL_GEL_H
#define GEL_GEL_H

#include "engine/game.h"
#include "gel/git.h"

class Gel : public eng::Game {
 public:
  Gel();
  ~Gel() override;

  bool Initialize() override;

  void Update(float delta_time) override;

 private:
  Git git_;
};

#endif  // GEL_GEL_H
