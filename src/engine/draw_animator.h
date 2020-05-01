#ifndef DRAW_ANIMATOR_H
#define DRAW_ANIMATOR_H

#include "../base/vecmath.h"
#include <cstdlib>
#include <vector>
#include <functional>

namespace engine {

class Drawable;

class DrawAnimator {
 public:
  using Callback = std::function<void()>;

  DrawAnimator() = default;
  ~DrawAnimator() = default;

  void AttachDrawable(Drawable *drawable);

  void SetMovement(Vector2 dir, float distance);

  void SetSpeed(float speed);

  void SetCallback(Callback c);

  void Play(bool loop);
  void Pause();
  void Stop();

  void Update(float delta_time);

  bool IsPlaying() const { return is_playing_; }

 private:
  struct DrawableTraits {
    Drawable* drawable;
    Vector2 start_pos;
  };

  bool is_playing_ = false;
  bool loop_ = false;

  std::vector<DrawableTraits> drawables_;

  // Movement speed per second.
  float speed_ = 0.002f;

  Vector2 dir_ = {0, 0};
  float distance_ = 0.0f;
  float movement_ = 0.0f;

  bool call_callback_ = false;
  Callback callback_;
};

}  // namespace engine

#endif  // DRAW_ANIMATOR_H
