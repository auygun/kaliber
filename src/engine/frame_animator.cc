#include "frame_animator.h"
#include "frame_controller.h"
#include <cassert>

namespace engine {

void FrameAnimator::AttachFrameController(FrameController *controller) {
  controllers_.push_back(controller);
}

void FrameAnimator::SetFrameRange(size_t start_frame, size_t end_frame) {
  start_frame_ = start_frame;
  end_frame_ = end_frame;
}

void FrameAnimator::SetIdleFrame(size_t idle_frame) {
  idle_frame_ = idle_frame;
  if (state_ == kStopped) {
    for (auto controller : controllers_)
      controller->SetCurrentFrame(idle_frame);
  }
}

void FrameAnimator::SetSpeed(float speed) {
  speed_ = speed;
}

void FrameAnimator::Play(bool loop) {
  loop_ = loop;
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
  if (state_ == kPaused)
    return;

  seconds_accumulated_ += delta_time;
  if (seconds_accumulated_ <= speed_)
    return;
  seconds_accumulated_ = 0;

  for (auto controller : controllers_) {
    switch (state_) {
    case kStopped:
      if (controller->GetCurrentFrame() == idle_frame_)
        break;
      assert(idle_frame_ >= start_frame_ && idle_frame_ < end_frame_);
      // Fall through.

    case kPlaying: {
      size_t ef = end_frame_ > 0 ? end_frame_ : controller->GetNumFrames();
      int next = controller->GetCurrentFrame() + 1;
      controller->SetCurrentFrame(next >= ef ? start_frame_ : next);
      if (!loop_ && controller->GetCurrentFrame() == idle_frame_)
        state_ = kStopped;
      break;
    }

    default:
      break;
    }
  }
}

}  // namespace engine
