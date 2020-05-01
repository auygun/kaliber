#include "alpha_animator.h"
#include "drawable.h"
#include <cassert>

namespace engine {

void AlphaAnimator::AttachDrawable(Drawable *drawable) {
  drawables_.push_back(drawable);
}

void AlphaAnimator::FadeOut() {
  is_playing_ = true;
  current_alpha_ = 1;
  for (auto& dt : drawables_)
    dt->SetVisible(true);
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
    for (auto& dt : drawables_)
      dt->SetVisible(false);
  } else {
    for (auto& dt : drawables_)
      dt->SetColor({1, 1, 1, current_alpha_});
  }
}

}  // namespace engine
