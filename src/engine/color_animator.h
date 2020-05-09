#ifndef COLOR_ANIMATOR_H
#define COLOR_ANIMATOR_H

#include "../base/vecmath.h"
#include <cstdlib>
#include <vector>
#include <functional>

namespace eng {

class ImageQuad;

class ColorAnimator {
 public:
  using Callback = std::function<void()>;

  ColorAnimator() = default;
  ~ColorAnimator() = default;

  void Attach(ImageQuad *quad);

  void SetTarget(Vector4 target_color, float speed);

  void SetCallback(Callback c);

  void Play();

  void Update(float delta_time);

  bool IsPlaying() const { return is_playing_; }

 private:
  struct AnimatableTraits {
    ImageQuad* quad;
    Vector4 start_color = {0, 0, 0, 0};
  };
  bool is_playing_ = false;
  std::vector<AnimatableTraits> animatables_;
  Vector4 target_color_ = {0, 0, 0, 0};
  float speed_;
  float acumulated_time_ = 0;
  Callback callback_;

  Vector4 BlendColors(Vector4 c1, Vector4 c2, float t);

  float BlendColorChannel(float c1, float c2, float t);
  float BlendAlphaChannel(float a1, float a2, float t);
};

}  // namespace eng

#endif  // COLOR_ANIMATOR_H
