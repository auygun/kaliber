#ifndef ANIMATOR_H
#define ANIMATOR_H

#include <vector>

#include "../base/closure.h"
#include "../base/vecmath.h"

namespace eng {

class Animatable;

class Animator {
 public:
  // Animation type flags.
  enum Flags {
    kNone = 0,
    kMovement = 1,
    kRotation = 2,
    kBlending = 4,
    kFrames = 8,
    kTimer = 16,
    kAllAnimations = kMovement | kRotation | kBlending | kFrames
  };

  using Interpolator = std::function<float(float)>;

  Animator();
  ~Animator();

  void Attach(Animatable* animatable);

  void Play(int animation, bool loop);
  void Pause(int animation);
  void Stop(int animation);

  void PauseOrResumeAll(bool pause);

  // Get/set current time of the given animation.
  float GetTime(int animation);
  void SetTime(int animation, float time, bool force_update = false);

  // Set callback ro be called once animation ends.
  void SetEndCallback(int animation, base::Closure cb);

  // Distance is the magnitude of direction vector. Duration is in seconds.
  void SetMovement(base::Vector2f direction,
                   float duration,
                   Interpolator interpolator = nullptr);

  // Rotation is in radian. Duration is in seconds.
  void SetRotation(float target,
                   float duration,
                   Interpolator interpolator = nullptr);

  void SetBlending(base::Vector4f target,
                   float duration,
                   Interpolator interpolator = nullptr);

  // Plays count number of frames.
  void SetFrames(int count,
                 int frames_per_second,
                 Interpolator interpolator = nullptr);

  // Triggers a callback after the given seconds passed.
  void SetTimer(float duration);

  // Set visibility of all attached animatables.
  void SetVisible(bool visible);

  void Update(float delta_time);
  void EvalAnim(float frame_time);

  bool IsPlaying(int animation) const { return play_flags_ & animation; }

 private:
  struct Element {
    Animatable* animatable;
    base::Vector2f movement_last_pos = {0, 0};
    float rotation_last_theta = 0;
    base::Vector4f blending_start = {0, 0, 0, 0};
    int frame_start_ = 0;
  };

  unsigned int play_flags_ = 0;
  unsigned int loop_flags_ = 0;
  unsigned int resume_flags_ = 0;
  std::vector<Element> elements_;

  base::Vector2f movement_direction_ = {0, 0};
  float movement_speed_ = 0;
  float movement_time_ = 0;
  Interpolator movement_interpolator_;
  base::Closure movement_cb_;

  float rotation_target_ = 0;
  float rotation_speed_ = 0;
  float rotation_time_ = 0;
  Interpolator rotation_interpolator_;
  base::Closure rotation_cb_;

  base::Vector4f blending_target_ = {0, 0, 0, 0};
  float blending_speed_ = 0;
  float blending_time_ = 0;
  Interpolator blending_interpolator_;
  base::Closure blending_cb_;

  int frame_count_ = 0;
  float frame_speed_ = 0;
  float frame_time_ = 0;
  Interpolator frame_interpolator_;
  base::Closure frame_cb_;

  float timer_speed_ = 0;
  float timer_time_ = 0;
  base::Closure timer_cb_;

  // State used to set new callback during a callback.
  bool has_pending_cb_ = false;
  base::Closure pending_cb_;
  Flags inside_cb_ = kNone;

  void UpdateMovement(float delta_time);
  void UpdateRotation(float delta_time);
  void UpdateBlending(float delta_time);
  void UpdateFrame(float delta_time);
  void UpdateTimer(float delta_time);
};

}  // namespace eng

#endif  // ANIMATOR_H
