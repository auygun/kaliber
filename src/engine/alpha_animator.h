#ifndef ALPHA_ANIMATOR_H
#define ALPHA_ANIMATOR_H

#include "../base/vecmath.h"
#include <cstdlib>
#include <vector>

namespace engine {

class ImageQuad;

class AlphaAnimator {
 public:
  AlphaAnimator() = default;
  ~AlphaAnimator() = default;

  void Attach(ImageQuad *quad);

  void SetSpeed(float speed);

  void FadeOut();

  void Update(float delta_time);

  bool IsPlaying() const { return is_playing_; }

 private:
  bool is_playing_ = false;
  std::vector<ImageQuad*> quads_;
  float current_alpha_ = 1;
  float speed_ = 4;
};

}  // namespace engine

#endif  // ALPHA_ANIMATOR_H
