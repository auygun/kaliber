#include "animator.h"

#include "../base/interpolation.h"
#include "../base/log.h"
#include "animatable.h"
#include "engine.h"

using namespace base;

namespace eng {

Animator::Animator() {
  Engine::Get().AddAnimator(this);
}

Animator::~Animator() {
  Engine::Get().RemoveAnimator(this);
}

void Animator::Attach(Animatable* animatable) {
  elements_.push_back({animatable,
                       {0, 0},
                       0,
                       animatable->GetColor(),
                       (int)animatable->GetFrame()});
}

void Animator::Play(int animation, bool loop) {
  play_flags_ |= animation;
  if (loop)
    loop_flags_ |= animation;
  else
    loop_flags_ &= ~animation;
  if ((loop_flags_ & kTimer) != 0)
    loop_flags_ &= ~kTimer;
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
  Evaluate(0);
  play_flags_ &= ~animation;
  loop_flags_ &= ~animation;
}

void Animator::PauseOrResumeAll(bool pause) {
  if (pause) {
    resume_flags_ = play_flags_;
    play_flags_ = 0;
  } else {
    play_flags_ = resume_flags_;
    resume_flags_ = 0;
  }
}

float Animator::GetTime(int animation) {
  if ((animation & kMovement) != 0)
    return movement_time_;
  if ((animation & kRotation) != 0)
    return rotation_time_;
  if ((animation & kBlending) != 0)
    return blending_time_;
  if ((animation & kFrames) != 0)
    return frame_time_;
  return timer_time_;
}

void Animator::SetTime(int animation, float time, bool force_update) {
  DCHECK(time >= 0 && time <= 1);

  if ((animation & kMovement) != 0)
    movement_time_ = time;
  if ((animation & kRotation) != 0)
    rotation_time_ = time;
  if ((animation & kBlending) != 0)
    blending_time_ = time;
  if ((animation & kFrames) != 0)
    frame_time_ = time;
  if ((animation & kTimer) != 0)
    timer_time_ = time;

  if (force_update) {
    unsigned play_flags = play_flags_;
    play_flags_ = animation;
    Update(0);
    Evaluate(0);
    play_flags_ = play_flags;
  }
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

void Animator::SetMovement(Vector2f direction,
                           float duration,
                           Interpolator interpolator) {
  movement_direction_ = direction;
  movement_speed_ = 1.0f / duration;
  movement_interpolator_ = std::move(interpolator);

  for (auto& a : elements_)
    a.movement_last_pos = {0, 0};
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

void Animator::SetBlending(Vector4f target,
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
    UpdateAnimTime(delta_time, kMovement, movement_speed_, movement_time_,
                   movement_cb_);
  if (play_flags_ & kRotation)
    UpdateAnimTime(delta_time, kRotation, rotation_speed_, rotation_time_,
                   rotation_cb_);
  if (play_flags_ & kBlending)
    UpdateAnimTime(delta_time, kBlending, blending_speed_, blending_time_,
                   blending_cb_);
  if (play_flags_ & kFrames)
    UpdateAnimTime(delta_time, kFrames, frame_speed_, frame_time_, frame_cb_);
  if (play_flags_ & kTimer)
    UpdateAnimTime(delta_time, kTimer, timer_speed_, timer_time_, timer_cb_);
}

void Animator::Evaluate(float frame_frac_time) {
  Vector2f pos = {0, 0};
  if (play_flags_ & kMovement) {
    float time = movement_time_ + movement_speed_ * frame_frac_time;
    float interpolated_time =
        movement_interpolator_ ? movement_interpolator_(time) : time;
    pos = base::Lerp({0, 0}, movement_direction_, interpolated_time);
  }

  float theta = 0;
  if (play_flags_ & kRotation) {
    float time = rotation_time_ + rotation_speed_ * frame_frac_time;
    float interpolated_time =
        rotation_interpolator_ ? rotation_interpolator_(time) : time;
    theta = base::Lerp(0.0f, rotation_target_, interpolated_time);
  }

  float blending_itime = 0;
  if (play_flags_ & kBlending) {
    float time = blending_time_ + blending_speed_ * frame_frac_time;
    blending_itime =
        blending_interpolator_ ? blending_interpolator_(time) : time;
  }

  float frame_itime = 0;
  if (play_flags_ & kFrames) {
    float time = frame_time_ + frame_speed_ * frame_frac_time;
    frame_itime = frame_interpolator_ ? frame_interpolator_(time) : time;
  }

  for (auto& a : elements_) {
    if (play_flags_ & kMovement) {
      a.animatable->Translate(pos - a.movement_last_pos);
      a.movement_last_pos = pos;
    }

    if (play_flags_ & kRotation) {
      a.animatable->Rotate(theta - a.rotation_last_theta);
      a.rotation_last_theta = theta;
    }

    if (play_flags_ & kBlending) {
      Vector4f blending =
          base::Lerp(a.blending_start, blending_target_, blending_itime);
      a.animatable->SetColor(blending);
    }

    if (play_flags_ & kFrames) {
      int target = a.frame_start_ + frame_count_;
      int frame = base::Lerp(a.frame_start_, target, frame_itime);
      if (frame < target)
        a.animatable->SetFrame(frame);
    }
  }
}

void Animator::UpdateAnimTime(float delta_time,
                              int anim,
                              float anim_speed,
                              float& anim_time,
                              base::Closure& cb) {
  if ((loop_flags_ & anim) == 0 && anim_time == 1.0f) {
    anim_time = 0;
    play_flags_ &= ~anim;
    if (cb) {
      inside_cb_ = (Flags)anim;
      cb();
      inside_cb_ = kNone;
      if (has_pending_cb_) {
        has_pending_cb_ = false;
        cb = std::move(pending_cb_);
      }
    }
    return;
  } else if ((anim & kFrames) != 0 && (loop_flags_ & kFrames) != 0 &&
             anim_time == 1.0f) {
    anim_time = 0;
  }

  anim_time += anim_speed * delta_time;
  if (anim_time > 1)
    anim_time = (anim & kFrames) != 0 || (anim & kTimer) != 0 ||
                        (loop_flags_ & anim) == 0
                    ? 1
                    : fmod(anim_time, 1.0f);
}

}  // namespace eng
