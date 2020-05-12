#include "animator.h"
#include "../base/log.h"
#include "../base/misc.h"
#include "image_quad.h"
#include <cmath>

namespace eng {

void Animator::Attach(ImageQuad *quad) {
  animatables_.push_back({quad,
                          quad->GetOffset(),
                          quad->GetTheta(),
                          quad->GetColor(),
                          (int)quad->GetFrame()});
}

void Animator::Play(Flags animation, bool loop) {
  play_flags_ |= animation;
  loop_flags_ |= loop ? animation : 0;
}

void Animator::Pause(Flags animation) {
  play_flags_ &= ~animation;
}

void Animator::Stop(Flags animation) {
  movement_time_ = 0;
  rotation_time_ = 0;
  blending_time_ = 0;
  frame_time_ = 0;

  Update(0);

  play_flags_ &= ~animation;
}

void Animator::UpdateStartValues(Flags animation) {
  for (auto& a : animatables_) {
    if ((animation & kMovement) != 0)
      a.movement_start = a.quad->GetOffset();
    if ((animation & kRotation) != 0)
      a.rotation_start_ = a.quad->GetTheta();
    if ((animation & kBlending) != 0)
      a.blending_start = a.quad->GetColor();
    if ((animation & kFrames) != 0)
      a.frame_start_ = a.quad->GetFrame();
  }
}

void Animator::SetEndCallback(Flags animation, Callback cb) {
  if ((animation & kMovement) != 0)
    movement_cb_ = cb;
  if ((animation & kRotation) != 0)
    rotation_cb_ = cb;
  if ((animation & kBlending) != 0)
    blending_cb_ = cb;
  if ((animation & kFrames) != 0)
    frame_cb_ = cb;
}

void Animator::SetMovement(Vector2 direction, float speed) {
  movement_direction_ = direction;
  float len = direction.Magnitude();
  movement_speed_ = speed / len;
}

void Animator::SetRotation(float target, float speed) {
  rotation_target_ = target;
  rotation_speed_ = speed / target;
}

void Animator::SetBlending(Vector4 target, float speed) {
  blending_target_ = target;
  blending_speed_ = speed;
}

void Animator::SetFrames(int count, int speed) {
  frame_count_ = count;
  frame_speed_ = (float)speed / (float)count;
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

  for (auto& a : animatables_) {
    if (play_flags_ & kMovement) {
      Vector2 target = a.movement_start + movement_direction_;
      Vector2 r = Lerp(a.movement_start, target, movement_time_);
      a.quad->SetOffset(r);
    }

    if (play_flags_ & kRotation) {
      float r = Lerp(a.rotation_start_, rotation_target_, rotation_time_);
      a.quad->SetTheta(r);
    }

    if (play_flags_ & kBlending) {
      Vector4 r = Blend(a.blending_start, blending_target_, blending_time_);
      a.quad->SetColor(r);
    }

    if (play_flags_ & kFrames) {
      int target = a.frame_start_ + frame_count_;
      int r = Lerp(a.frame_start_, target, frame_time_);
      if (r < target)
        a.quad->SetFrame(r);
    }
  }
}

void Animator::UpdateMovement(float delta_time) {
  if ((loop_flags_ & kMovement) == 0 && movement_time_ == 1.0f) {
    movement_time_ = 0;
    play_flags_ &= ~kMovement;
    if (movement_cb_)
      movement_cb_();
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
    if (rotation_cb_)
      rotation_cb_();
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
    if (blending_cb_)
      blending_cb_();
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
    if (frame_cb_)
      frame_cb_();
    return;
  } else if ((loop_flags_ & kFrames) != 0 && frame_time_ == 1.0f) {
    frame_time_ = 0;
  }

  frame_time_ += frame_speed_ * delta_time;
  if (frame_time_ > 1)
    frame_time_ = 1;
}

}  // namespace eng
