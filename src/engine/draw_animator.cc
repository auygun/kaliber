#include "draw_animator.h"
#include "drawable.h"
#include "../base/log.h"
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

void DrawAnimator::SetCallback(Callback callback) {
  call_callback_ = true;
  callback_ = std::move(callback);
}

void DrawAnimator::Play(bool loop) {
  if (is_playing_)
    return;
  is_playing_ = true;
  loop_ = loop;
}

void DrawAnimator::Pause() {
  is_playing_ = false;
}

void DrawAnimator::Stop() {
  is_playing_ = false;
  movement_ = 0;
  for (auto& dt : drawables_)
    dt.drawable->SetOffset(dt.start_pos);
}

void DrawAnimator::Update(float delta_time) {
  if (!is_playing_)
    return;

  Vector2 offset = dir_ * speed_ * delta_time;
  movement_ += offset.Magnitude();
  if (movement_ <= distance_) {
    for (auto& dt : drawables_)
      dt.drawable->Translate(offset);
  } else if (loop_) {
    movement_ = 0;
    for (auto& dt : drawables_)
      dt.drawable->SetOffset(dt.start_pos);
    if (call_callback_)
      callback_();
  } else {
    is_playing_ = false;
    if (call_callback_)
      callback_();
  }
}

}  // namespace engine
