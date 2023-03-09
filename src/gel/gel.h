#ifndef GEL_GEL_H
#define GEL_GEL_H

#include <memory>

#include "engine/animator.h"
#include "engine/game.h"
#include "engine/solid_quad.h"
#include "gel/proc_runner.h"

class Gel : public eng::Game {
 public:
  Gel();
  ~Gel() override;

  bool Initialize() override;

  void Update(float delta_time) override;

  void ContextLost() override;

  void LostFocus() override;

  void GainedFocus(bool from_interstitial_ad) override;

 private:
  eng::SolidQuad quad_;
  eng::Animator animator_;

  ProcRunner proc_runner_;

  void OnGitOutput(int pid, std::string line);
  void OnGitFinished(int pid,
                     base::Exec::Status status,
                     int result,
                     std::string err);
};

#endif  // GEL_GEL_H
