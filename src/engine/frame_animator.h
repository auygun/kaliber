#ifndef FRAME_ANIMATOR_H
#define FRAME_ANIMATOR_H

#include "../base/vecmath.h"
#include <cstdlib>
#include <vector>

namespace engine {

class FrameController;

class FrameAnimator {
 public:
  FrameAnimator() = default;
  ~FrameAnimator() = default;

  void AttachDrawable(FrameController *drawable);

  void SetFrameRange(size_t start_frame, size_t end_frame);
  void SetIdleFrame(size_t idle_frame);

  // Time in seconds between frames.
  void SetSpeed(float speed);

  void Play();
  void Pause();
  void Stop();

  void Update(float delta_time);

 private:
  enum State { kStopped, kPlaying, kPaused };

  State state_ = kStopped;

  std::vector<FrameController*> animatables_;

  float seconds_accumulated_ = 0.0f;
  float speed_ = 0.1f;

  size_t idle_frame_ = 0;
  size_t start_frame_ = 0;
  size_t end_frame_ = 0;
};

}  // namespace engine

#endif  // FRAME_ANIMATOR_H
