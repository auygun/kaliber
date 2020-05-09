#include "../base/log.h"
#include "color_animator.h"
#include "image_quad.h"
#include <math.h>
#include <cassert>

namespace eng {

void ColorAnimator::Attach(ImageQuad *quad) {
  animatables_.push_back({quad});
}

void ColorAnimator::Play() {
  is_playing_ = true;
  acumulated_time_ = 0;
  for (auto& a : animatables_)
    a.start_color = a.quad->GetColor();
}

void ColorAnimator::SetTarget(Vector4 target_color, float speed) {
  target_color_ = target_color;
  speed_ = speed;
}

void ColorAnimator::SetCallback(Callback callback) {
  callback_ = std::move(callback);
}

void ColorAnimator::Update(float delta_time) {
  if (!is_playing_)
    return;

  acumulated_time_ += speed_ * delta_time;
  if (acumulated_time_ > 1) {
    acumulated_time_ = 1;
    is_playing_ = false;
  }

  for (auto& a : animatables_) {
    Vector4 c = BlendColors(a.start_color, target_color_, acumulated_time_);
    a.quad->SetColor(c);
  }

  if (!is_playing_ && callback_)
    callback_();
}

Vector4 ColorAnimator::BlendColors(Vector4 c1, Vector4 c2, float t) {
  return Vector4(BlendColorChannel(c1.x, c2.x, t),
                 BlendColorChannel(c1.y, c2.y, t),
                 BlendColorChannel(c1.z, c2.z, t),
                 BlendAlphaChannel(c1.w, c2.w, t));
}

float ColorAnimator::BlendColorChannel(float c1, float c2, float t) {
  return sqrt((1 - t) * c1 * c1 + t * c2 * c2);
}

float ColorAnimator::BlendAlphaChannel(float a1, float a2, float t) {
  return (1 - t) * a1 + t * a2;
}

}  // namespace eng
