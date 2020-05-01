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

  void AttachFrameController(FrameController *drawable);

  void SetFrameRange(size_t start_frame, size_t end_frame);
  void SetIdleFrame(size_t idle_frame);

  void SetSpeed(float speed);

  void Play(bool loop);
  void Pause();
  void Stop();

  void Update(float delta_time);

 private:
  enum State { kStopped, kPlaying, kPaused };

  State state_ = kStopped;
  bool loop_ = false;

  std::vector<FrameController*> controllers_;

  // Time in seconds between frames.
  float speed_ = 0.1f;
  float seconds_accumulated_ = 0.0f;

  size_t idle_frame_ = 0;
  size_t start_frame_ = 0;
  size_t end_frame_ = 0;
};

}  // namespace engine

#endif  // FRAME_ANIMATOR_H
