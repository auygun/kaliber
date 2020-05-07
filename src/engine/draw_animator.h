#ifndef DRAW_ANIMATOR_H
#define DRAW_ANIMATOR_H

#include "../base/vecmath.h"
#include <cstdlib>
#include <vector>
#include <functional>

namespace engine {

class ImageQuad;

class DrawAnimator {
 public:
  using Callback = std::function<void()>;

  DrawAnimator() = default;
  ~DrawAnimator() = default;

  void Attach(ImageQuad *quad);

  void SetMovement(Vector2 dir, float distance);

  void SetRotation(float theta);

  void SetSpeed(float speed);

  void SetCallback(Callback c);

  void Play(bool loop);
  void Pause();
  void Stop();

  void Update(float delta_time);

  bool IsPlaying() const { return is_playing_; }

 private:
  struct QuadTraits {
    ImageQuad* quad;
    Vector2 start_pos;
  };

  enum Flags { kFlag_Movement = 1, kFlag_Rotation = 2 };

  unsigned int flags_ = 0;

  bool is_playing_ = false;
  bool loop_ = false;

  std::vector<QuadTraits> quads_;

  float theta_ = 0.0f;
  float rotation_ = 0.0f;

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
