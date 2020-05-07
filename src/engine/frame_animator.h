#ifndef FRAME_ANIMATOR_H
#define FRAME_ANIMATOR_H

#include "../base/vecmath.h"
#include <cstdlib>
#include <vector>
#include <functional>

namespace eng {

class ImageQuad;

class FrameAnimator {
 public:
  using Callback = std::function<void()>;

  FrameAnimator() = default;
  ~FrameAnimator() = default;

  void Attach(ImageQuad *quad);

  void SetFrameRange(size_t start_frame, size_t end_frame, size_t idle_frame);

  void SetSpeed(float speed);

  void SetCallback(size_t frame, Callback c);

  void Play(bool loop, bool reset);
  void Pause();
  void Stop();

  void Update(float delta_time);

  bool IsPlaying() const { return state_ == kPlaying; }

  size_t idle_frame() { return idle_frame_; }

 private:
  enum State { kStopped, kPlaying, kPaused };

  State state_ = kStopped;
  bool loop_ = false;

  std::vector<ImageQuad*> quads_;

  // Time in seconds between frames.
  float speed_ = 0.1f;
  float seconds_accumulated_ = 0.0f;

  size_t idle_frame_ = 0;
  size_t start_frame_ = 0;
  size_t end_frame_ = 0;

  size_t callback_frame_ = 0;
  Callback callback_;
};

}  // namespace eng

#endif  // FRAME_ANIMATOR_H
