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
  flags_ |= kFlag_Movement;
}

void DrawAnimator::SetRotation(float theta) {
  theta_ = theta;
  flags_ |= kFlag_Rotation;
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

  bool do_callback = false;
  Vector2 offset = {0, 0};

  if (flags_ & kFlag_Movement) {
    offset = dir_ * speed_ * delta_time;
    movement_ += offset.Magnitude();
    if (movement_ <= distance_) {
    } else if (loop_) {
      movement_ = 0;
      do_callback = true;
    } else {
      is_playing_ = false;
      do_callback = true;
    }
  }
  if (flags_ & kFlag_Rotation) {
    rotation_ += theta_ * delta_time;
  }

  if (is_playing_) {
    for (auto& dt : drawables_) {
      if (flags_ & kFlag_Movement) {
        if (movement_ == 0.0f)
          dt.drawable->SetOffset(dt.start_pos);
        else
          dt.drawable->Translate(offset);
      }
      if (flags_ & kFlag_Rotation)
        dt.drawable->Rotate(rotation_);
    }
  }

  if (do_callback && call_callback_)
    callback_();
}

}  // namespace engine
