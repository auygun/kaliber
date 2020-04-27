#include "frame_animator.h"
#include "frame_controller.h"
#include <cassert>

namespace engine {

void FrameAnimator::AttachDrawable(FrameController *animatable) {
  animatables_.push_back(animatable);
}

void FrameAnimator::SetFrameRange(size_t start_frame, size_t end_frame) {
  start_frame_ = start_frame;
  end_frame_ = end_frame;
}

void FrameAnimator::SetIdleFrame(size_t idle_frame) {
  idle_frame_ = idle_frame;
}

void FrameAnimator::SetSpeed(float speed) {
  speed_ = speed;
}

void FrameAnimator::Play() {
  if (state_ == kPlaying)
    return;
  state_ = kPlaying;
  seconds_accumulated_ = 0.0f;
}

void FrameAnimator::Pause() {
  state_ = kPaused;
}

void FrameAnimator::Stop() {
  state_ = kStopped;
}

void FrameAnimator::Update(float delta_time) {
  for (auto animatable : animatables_) {
    switch (state_) {
    case kPaused:
      break;
    case kStopped:
      if (animatable->GetCurrentFrame() == idle_frame_)
        break;
      assert(idle_frame_ >= start_frame_ && idle_frame_ < end_frame_);
      // Fall through.
    case kPlaying:
      seconds_accumulated_ += delta_time;
      if (seconds_accumulated_ > speed_) {
        seconds_accumulated_ = 0;
        size_t ef = end_frame_ > 0 ? end_frame_ : animatable->GetNumFrames();
        int next = animatable->GetCurrentFrame() + 1;
        animatable->SetCurrentFrame(next >= ef ? start_frame_ : next);
      }
    }
  }
}

}  // namespace engine
