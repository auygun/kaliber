#include "frame_animator.h"
#include "image_quad.h"
#include <cassert>

namespace eng {

void FrameAnimator::Attach(ImageQuad *quad) {
  quads_.push_back(quad);
}

void FrameAnimator::SetFrameRange(size_t start_frame,
                                  size_t end_frame,
                                  size_t idle_frame) {
  start_frame_ = start_frame;
  end_frame_ = end_frame;
  idle_frame_ = idle_frame;
  if (state_ == kStopped) {
    for (auto q : quads_)
      q->SetFrame(start_frame_);
  }
}

void FrameAnimator::SetSpeed(float speed) {
  speed_ = speed;
}

void FrameAnimator::SetCallback(size_t frame, Callback callback) {
  callback_frame_ = frame;
  callback_ = std::move(callback);
}

void FrameAnimator::Play(bool loop, bool reset) {
  if (state_ != kPlaying)
    seconds_accumulated_ = 0.0f;
  state_ = kPlaying;
  loop_ = loop;
  if (reset) {
    seconds_accumulated_ = 0.0f;
    for (auto q : quads_)
      q->SetFrame(start_frame_);
  }
}

void FrameAnimator::Pause() {
  state_ = kPaused;
}

void FrameAnimator::Stop() {
  state_ = kStopped;
}

void FrameAnimator::Update(float delta_time) {
  if (state_ == kPaused)
    return;

  seconds_accumulated_ += delta_time;
  if (seconds_accumulated_ <= speed_)
    return;
  seconds_accumulated_ = 0;

  for (auto q : quads_) {
    switch (state_) {
    case kStopped:
      if (q->GetFrame() == idle_frame_)
        break;
      assert(idle_frame_ >= start_frame_ && idle_frame_ < end_frame_);
      // Fall through.

    case kPlaying: {
      int next = q->GetFrame() + 1;
      q->SetFrame(next >= end_frame_ ? start_frame_ : next);
      if (callback_frame_ != 0 && q->GetFrame() == start_frame_ + callback_frame_)
        callback_();
      if (!loop_ && q->GetFrame() == idle_frame_)
        state_ = kStopped;
      break;
    }

    default:
      break;
    }
  }
}

}  // namespace eng
