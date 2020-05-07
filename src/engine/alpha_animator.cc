#include "alpha_animator.h"
#include "image_quad.h"
#include <cassert>

namespace engine {

void AlphaAnimator::Attach(ImageQuad *quad) {
  quads_.push_back(quad);
}

void AlphaAnimator::FadeOut() {
  is_playing_ = true;
  current_alpha_ = 1;
  for (auto& q : quads_)
    q->SetVisible(true);
}

void AlphaAnimator::SetSpeed(float speed) {
  speed_ = speed;
}

void AlphaAnimator::Update(float delta_time) {
  if (!is_playing_)
    return;

  current_alpha_ -= delta_time * speed_;
  if (current_alpha_ <= 0) {
    current_alpha_ = 0;
    is_playing_ = false;
    for (auto& q : quads_)
      q->SetVisible(false);
  } else {
    for (auto& q : quads_)
      q->SetColor({1, 1, 1, current_alpha_});
  }
}

}  // namespace engine
