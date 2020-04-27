#include "draw_animator.h"
#include "drawable.h"
#include <cassert>

namespace engine {

void DrawAnimator::AttachDrawable(Drawable *drawable) {
  drawables_.push_back({drawable, drawable->offset()});
}

void DrawAnimator::SetMovement(Vector2 dir, float distance) {
  dir_ = dir;
  dir_.Normalize();
  distance_ = distance;
}

void DrawAnimator::SetSpeed(float speed) {
  speed_ = speed;
}

void DrawAnimator::Play() {
  if (is_playing_)
    return;
  is_playing_ = true;
}

void DrawAnimator::Pause() {
  is_playing_ = false;
}

void DrawAnimator::Update(float delta_time) {
  if (!is_playing_)
    return;

  Vector2 offset = dir_ * speed_;
  movement_ += offset.Magnitude();
  if (movement_ > distance_) {
    movement_ = 0;
    for (auto& dt : drawables_)
      dt.drawable->SetOffset(dt.start_pos);
  } else {
    for (auto& dt : drawables_)
      dt.drawable->Translate(offset);
  }
}

}  // namespace engine
