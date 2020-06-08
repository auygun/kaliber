#include "animator.h"

#include "../base/interpolation.h"
#include "../base/log.h"
#include "animatable.h"

using namespace base;

namespace eng {

void Animator::Attach(Animatable* animatable) {
  elements_.push_back({animatable,
                       {0, 0},
                       0,
                       animatable->GetColor(),
                       (int)animatable->GetFrame()});
}

void Animator::Play(int animation, bool loop) {
  play_flags_ |= animation;
  loop_flags_ |= loop ? animation : 0;
}

void Animator::Pause(int animation) {
  play_flags_ &= ~animation;
}

void Animator::Stop(int animation) {
  if ((animation & kMovement) != 0)
    movement_time_ = 0;
  if ((animation & kRotation) != 0)
    rotation_time_ = 0;
  if ((animation & kBlending) != 0)
    blending_time_ = 0;
  if ((animation & kFrames) != 0)
    frame_time_ = 0;
  if ((animation & kTimer) != 0)
    timer_time_ = 0;

  play_flags_ |= animation;
  Update(0);
  play_flags_ &= ~animation;
  loop_flags_ &= ~animation;
}

void Animator::SetEndCallback(int animation, base::Closure cb) {
  if ((inside_cb_ & animation) != 0) {
    has_pending_cb_ = true;
    pending_cb_ = std::move(cb);
  }
  if ((animation & kMovement) != 0 && inside_cb_ != kMovement)
    movement_cb_ = std::move(cb);
  if ((animation & kRotation) != 0 && inside_cb_ != kRotation)
    rotation_cb_ = std::move(cb);
  if ((animation & kBlending) != 0 && inside_cb_ != kBlending)
    blending_cb_ = std::move(cb);
  if ((animation & kFrames) != 0 && inside_cb_ != kFrames)
    frame_cb_ = std::move(cb);
  if ((animation & kTimer) != 0 && inside_cb_ != kTimer)
    timer_cb_ = std::move(cb);
}

void Animator::SetMovement(Vector2 direction,
                           float duration,
                           Interpolator interpolator) {
  movement_direction_ = direction;
  movement_speed_ = 1.0f / duration;
  movement_interpolator_ = std::move(interpolator);

  for (auto& a : elements_)
    a.movement_last_offset = {0, 0};
}

void Animator::SetRotation(float trget,
                           float duration,
                           Interpolator interpolator) {
  rotation_target_ = trget;
  rotation_speed_ = 1.0f / duration;
  rotation_interpolator_ = std::move(interpolator);

  for (auto& a : elements_)
    a.rotation_last_theta = 0;
}

void Animator::SetBlending(Vector4 target,
                           float duration,
                           Interpolator interpolator) {
  blending_target_ = target;
  blending_speed_ = 1.0f / duration;
  for (auto& a : elements_)
    a.blending_start = a.animatable->GetColor();
  blending_interpolator_ = std::move(interpolator);
}

void Animator::SetFrames(int count,
                         int frames_per_second,
                         Interpolator interpolator) {
  frame_count_ = count;
  frame_speed_ = (float)frames_per_second / (float)count;
  for (auto& a : elements_)
    a.frame_start_ = a.animatable->GetFrame();
  frame_interpolator_ = std::move(interpolator);
}

void Animator::SetTimer(float duration) {
  timer_speed_ = 1.0f / duration;
}

void Animator::SetVisible(bool visible) {
  for (auto& a : elements_)
    a.animatable->SetVisible(visible);
}

void Animator::Update(float delta_time) {
  if (play_flags_ & kMovement)
    UpdateMovement(delta_time);
  if (play_flags_ & kRotation)
    UpdateRotation(delta_time);
  if (play_flags_ & kBlending)
    UpdateBlending(delta_time);
  if (play_flags_ & kFrames)
    UpdateFrame(delta_time);
  if (play_flags_ & kTimer)
    UpdateTimer(delta_time);

  for (auto& a : elements_) {
    if (play_flags_ & kMovement) {
      float interpolated_time = movement_interpolator_
                                    ? movement_interpolator_(movement_time_)
                                    : movement_time_;
      Vector2 offset =
          base::Lerp({0, 0}, movement_direction_, interpolated_time);
      a.animatable->Translate(offset - a.movement_last_offset);
      a.movement_last_offset = offset;
    }

    if (play_flags_ & kRotation) {
      float interpolated_time = rotation_interpolator_
                                    ? rotation_interpolator_(rotation_time_)
                                    : rotation_time_;
      float theta = base::Lerp(0.0f, rotation_target_, interpolated_time);
      a.animatable->Rotate(theta - a.rotation_last_theta);
      a.rotation_last_theta = theta;
    }

    if (play_flags_ & kBlending) {
      float interpolated_time = blending_interpolator_
                                    ? blending_interpolator_(blending_time_)
                                    : blending_time_;
      Vector4 r =
          base::Lerp(a.blending_start, blending_target_, interpolated_time);
      a.animatable->SetColor(r);
    }

    if (play_flags_ & kFrames) {
      float interpolated_time =
          frame_interpolator_ ? frame_interpolator_(frame_time_) : frame_time_;
      int target = a.frame_start_ + frame_count_;
      int r = base::Lerp(a.frame_start_, target, interpolated_time);
      if (r < target)
        a.animatable->SetFrame(r);
    }
  }
}

void Animator::UpdateMovement(float delta_time) {
  if ((loop_flags_ & kMovement) == 0 && movement_time_ == 1.0f) {
    movement_time_ = 0;
    play_flags_ &= ~kMovement;
    if (movement_cb_) {
      inside_cb_ = kMovement;
      movement_cb_();
      inside_cb_ = kNone;
      if (has_pending_cb_) {
        has_pending_cb_ = false;
        movement_cb_ = std::move(pending_cb_);
      }
    }
    return;
  }

  movement_time_ += movement_speed_ * delta_time;
  if (movement_time_ > 1)
    movement_time_ =
        (loop_flags_ & kMovement) == 0 ? 1 : fmod(movement_time_, 1.0f);
}

void Animator::UpdateRotation(float delta_time) {
  if ((loop_flags_ & kRotation) == 0 && rotation_time_ == 1.0f) {
    rotation_time_ = 0;
    play_flags_ &= ~kRotation;
    if (rotation_cb_) {
      inside_cb_ = kRotation;
      rotation_cb_();
      inside_cb_ = kNone;
      if (has_pending_cb_) {
        has_pending_cb_ = false;
        rotation_cb_ = std::move(pending_cb_);
      }
    }
    return;
  }

  rotation_time_ += rotation_speed_ * delta_time;
  if (rotation_time_ > 1)
    rotation_time_ =
        (loop_flags_ & kRotation) == 0 ? 1 : fmod(rotation_time_, 1.0f);
}

void Animator::UpdateBlending(float delta_time) {
  if ((loop_flags_ & kBlending) == 0 && blending_time_ == 1.0f) {
    blending_time_ = 0;
    play_flags_ &= ~kBlending;
    if (blending_cb_) {
      inside_cb_ = kBlending;
      blending_cb_();
      inside_cb_ = kNone;
      if (has_pending_cb_) {
        has_pending_cb_ = false;
        blending_cb_ = std::move(pending_cb_);
      }
    }
    return;
  }

  blending_time_ += blending_speed_ * delta_time;
  if (blending_time_ > 1)
    blending_time_ =
        (loop_flags_ & kBlending) == 0 ? 1 : fmod(blending_time_, 1.0f);
}

void Animator::UpdateFrame(float delta_time) {
  if ((loop_flags_ & kFrames) == 0 && frame_time_ == 1.0f) {
    frame_time_ = 0;
    play_flags_ &= ~kFrames;
    if (frame_cb_) {
      inside_cb_ = kFrames;
      frame_cb_();
      inside_cb_ = kNone;
      if (has_pending_cb_) {
        has_pending_cb_ = false;
        frame_cb_ = std::move(pending_cb_);
      }
    }
    return;
  } else if ((loop_flags_ & kFrames) != 0 && frame_time_ == 1.0f) {
    frame_time_ = 0;
  }

  frame_time_ += frame_speed_ * delta_time;
  if (frame_time_ > 1)
    frame_time_ = 1;
}

void Animator::UpdateTimer(float delta_time) {
  if (timer_time_ == 1.0f) {
    timer_time_ = 0;
    play_flags_ &= ~kTimer;
    if (timer_cb_) {
      inside_cb_ = kTimer;
      timer_cb_();
      inside_cb_ = kNone;
      if (has_pending_cb_) {
        has_pending_cb_ = false;
        timer_cb_ = std::move(pending_cb_);
      }
    }
    return;
  }

  timer_time_ += timer_speed_ * delta_time;
  if (timer_time_ > 1)
    timer_time_ = 1;
}

}  // namespace eng
