#include "animator.h"
#include "../base/log.h"
#include "../base/misc.h"
#include "animatable.h"
#include <cmath>

using base::Vector2;
using base::Vector4;

namespace eng {

void Animator::Attach(Animatable *animatable) {
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
}

void Animator::SetEndCallback(int animation, base::Callback cb) {
  if ((animation & kMovement) != 0) {
    if (inside_cb_ == kMovement) {
      has_pending_cb_ = true;
      pending_cb_ = std::move(cb);
    } else {
      movement_cb_ = std::move(cb);
    }
  }
  if ((animation & kRotation) != 0) {
    if (inside_cb_ == kRotation) {
      has_pending_cb_ = true;
      pending_cb_ = std::move(cb);
    } else {
      rotation_cb_ = std::move(cb);
    }
  }
  if ((animation & kBlending) != 0) {
    if (inside_cb_ == kBlending) {
      has_pending_cb_ = true;
      pending_cb_ = std::move(cb);
    } else {
      blending_cb_ = std::move(cb);
    }
  }
  if ((animation & kFrames) != 0) {
    if (inside_cb_ == kFrames) {
      has_pending_cb_ = true;
      pending_cb_ = std::move(cb);
    } else {
      frame_cb_ = std::move(cb);
    }
  }
  if ((animation & kTimer) != 0) {
    if (inside_cb_ == kTimer) {
      has_pending_cb_ = true;
      pending_cb_ = std::move(cb);
    } else {
      timer_cb_ = std::move(cb);
    }
  }
}

void Animator::SetMovement(Vector2 direction, float speed) {
  movement_direction_ = direction;
  float len = direction.Magnitude();
  movement_speed_ = speed / len;
}

void Animator::SetRotation(float trget, float speed) {
  rotation_target_ = trget;
  rotation_speed_ = speed / trget;
}

void Animator::SetBlending(Vector4 target, float speed) {
  blending_target_ = target;
  blending_speed_ = 1.0f / speed;
  for (auto& a : elements_)
    a.blending_start = a.animatable->GetColor();
}

void Animator::SetFrames(int count, int speed) {
  frame_count_ = count;
  frame_speed_ = (float)speed / (float)count;
  for (auto& a : elements_)
    a.frame_start_ = a.animatable->GetFrame();
}

void Animator::SetTimer(float seconds) {
  timer_speed_ = 1.0f / seconds;
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
      Vector2 offset = base::Lerp({0, 0}, movement_direction_, movement_time_);
      a.animatable->Translate(offset - a.movement_last_offset);
      a.movement_last_offset = offset;
    }

    if (play_flags_ & kRotation) {
      float theta = base::Lerp(0.0f, rotation_target_, rotation_time_);
      a.animatable->Rotate(theta - a.rotation_last_theta);
      a.rotation_last_theta = theta;
    }

    if (play_flags_ & kBlending) {
      Vector4 r = base::Blend(a.blending_start, blending_target_, blending_time_);
      a.animatable->SetColor(r);
    }

    if (play_flags_ & kFrames) {
      int target = a.frame_start_ + frame_count_;
      int r = base::Lerp(a.frame_start_, target, frame_time_);
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
  if ((loop_flags_ & kMovement) == 0 && movement_time_ > 1)
    movement_time_ = 1;
  else
    movement_time_ = fmod(movement_time_, 1.0f);
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
  if ((loop_flags_ & kRotation) == 0 && rotation_time_ > 1)
    rotation_time_ = 1;
  else
    rotation_time_ = fmod(rotation_time_, 1.0f);
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
  if ((loop_flags_ & kBlending) == 0 && blending_time_ > 1)
    blending_time_ = 1;
  else
    blending_time_ = fmod(blending_time_, 1.0f);
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
