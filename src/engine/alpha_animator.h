#ifndef ALPHA_ANIMATOR_H
#define ALPHA_ANIMATOR_H

#include "../base/vecmath.h"
#include <cstdlib>
#include <vector>

namespace engine {

class Drawable;

class AlphaAnimator {
 public:
  AlphaAnimator() = default;
  ~AlphaAnimator() = default;

  void AttachDrawable(Drawable *drawable);

  void SetSpeed(float speed);

  void FadeOut();

  void Update(float delta_time);

  bool IsPlaying() const { return is_playing_; }

 private:
  bool is_playing_ = false;
  std::vector<Drawable*> drawables_;
  float current_alpha_ = 1;
  float speed_ = 4;
};

}  // namespace engine

#endif  // ALPHA_ANIMATOR_H
