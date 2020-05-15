#ifndef ANIMATOR_H
#define ANIMATOR_H

#include "../base/vecmath.h"
#include <vector>
#include <functional>

namespace eng {

class Animatable;

class Animator {
 public:
  // Animation type flags.
  enum Flags {
    kMovement = 1,
    kRotation = 2,
    kBlending = 4,
    kFrames = 8,
    kTimer = 16,
    kAllAnimations = kMovement | kRotation | kBlending | kFrames
  };

  using Callback = std::function<void()>;

  Animator() = default;
  ~Animator() = default;

  // Attached the given animatable to this animator and sets the start values.
  void Attach(Animatable *animatable);

  void Play(int animation, bool loop);
  void Pause(int animation);
  void Stop(int animation);

  // Set callback for the given animations. It's called for each animation once
  // it ends. Not that it's not called for looping animations.
  void SetEndCallback(int animation, Callback cb);

  // Set movement animation parameters. Movement animations is relative.
  // Distance is calculated from the magnitude of direction vector. Speed is in
  // movement per second.
  void SetMovement(Vector2 direction, float speed);

  // Set rotation animation parameters. Rotation animation is relative. Target
  // is in radians. Speed is in rotation per second.
  void SetRotation(float target, float speed);

  // Set color blending animation parameters. Color blending animation is
  // absolute. The absolute start colors are obtained from the attached
  // animatables. Speed is in seconds.
  void SetBlending(Vector4 target, float speed);

  // Set frame playback animation parameters. Frame animation is absolute. The
  // absolute start frames are obtained from the attached animatables. Plays
  // count number of frames. Speed is in frames per second.
  void SetFrames(int count, int speed);

  // Set Timer parameters. Timer doesn't play any animation. Usefull for
  // triggering a callback after the given seconds passed. Loop parameter is
  // ignored when played.
  void SetTimer(float seconds);

  void Update(float delta_time);

  bool IsPlaying(int animation) const { return play_flags_ & animation; }

 private:
  struct Element {
    Animatable* animatable;
    Vector2 movement_last_offset = {0, 0};
    float rotation_last_theta = 0;
    Vector4 blending_start = {0, 0, 0, 0};
    int frame_start_ = 0;
  };

  unsigned int play_flags_ = 0;
  unsigned int loop_flags_ = 0;
  std::vector<Element> elements_;

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

  float timer_speed_ = 0;
  float timer_time_ = 0;
  Callback timer_cb_;

  void UpdateMovement(float delta_time);
  void UpdateRotation(float delta_time);
  void UpdateBlending(float delta_time);
  void UpdateFrame(float delta_time);
  void UpdateTimer(float delta_time);
};

}  // namespace eng

#endif  // ANIMATOR_H
