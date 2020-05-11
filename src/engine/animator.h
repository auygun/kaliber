#ifndef ANIMATOR_H
#define ANIMATOR_H

#include "../base/vecmath.h"
#include <vector>
#include <functional>

namespace eng {

class ImageQuad;

class Animator {
 public:
  // Animation type flags.
  enum Flags {
    kMovement = 1,
    kRotation = 2,
    kBlending = 4,
    kFrames = 8,
    kAllAnimations = kMovement | kRotation | kBlending | kFrames
  };

  using Callback = std::function<void()>;

  Animator() = default;
  ~Animator() = default;

  // Attached the given animatable to this animator and sets the start values.
  void Attach(ImageQuad *quad);

  void Play(Flags animation, bool loop);
  void Pause(Flags animation);
  void Stop(Flags animation);

  // Updates start values for the given animations from attached animatables.
  void UpdateStartValues(Flags animation);

  // Set callback for the given animations. It's called for each animation once
  // it ends. Not that it's not called for looping animations.
  void SetEndCallback(Flags animation, Callback cb);

  // Set movement animation parameters. Distance is calculated from the
  // magnitude of direction vector. Speed is mevement per unit.
  void SetMovement(Vector2 direction, float speed);

  // Set rotation animation parameters. Target is in radians. Speed is rotation
  // per unit.
  void SetRotation(float target, float speed);

  // Set color blending animation parameters. Speed is in seconds.
  void SetBlending(Vector4 target, float speed);

  // Set frame playback animation parameters. Starts from the last updated
  // frames from attached animatables and plays count number of frames. Speed is
  // in frames per second.
  void SetFrames(int count, int speed);

  void Update(float delta_time);

  bool IsPlaying(Flags animation) const { return play_flags_ & animation; }

 private:
  struct Animatable {
    ImageQuad* quad;
    Vector2 movement_start = {0, 0};
    float rotation_start_ = 0;
    Vector4 blending_start = {0, 0, 0, 0};
    int frame_start_ = 0;
  };

  unsigned int play_flags_ = 0;
  unsigned int loop_flags_ = 0;
  std::vector<Animatable> animatables_;

  Vector2 movement_direction_ = {0, 0};
  float movement_speed_ = 0;
  float movement_time_ = 0;
  Callback movement_cb_;

  float rotation_target_ = 0;
  float rotation_speed_ = 0;
  float rotation_time_ = 0;
  Callback rotation_cb_;

  Vector4 blending_target_ = {0, 0, 0, 0};
  float blending_speed_ = 0;
  float blending_time_ = 0;
  Callback blending_cb_;

  int frame_count_ = 0;
  float frame_speed_ = 0;
  float frame_time_ = 0;
  Callback frame_cb_;

  void UpdateMovement(float delta_time);
  void UpdateRotation(float delta_time);
  void UpdateBlending(float delta_time);
  void UpdateFrame(float delta_time);
};

}  // namespace eng

#endif  // ANIMATOR_H
