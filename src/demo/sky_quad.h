#ifndef DEMO_SKY_QUAD_H
#define DEMO_SKY_QUAD_H

#include "base/vecmath.h"
#include "engine/animatable.h"
#include "engine/animator.h"

namespace eng {
class Shader;
}  // namespace eng

class SkyQuad : public eng::Animatable {
 public:
  SkyQuad();
  ~SkyQuad();

  SkyQuad(const SkyQuad&) = delete;
  SkyQuad& operator=(const SkyQuad&) = delete;

  bool Create(bool without_nebula);

  void Update(float delta_time);

  // Animatable interface.
  void SetFrame(size_t frame) final {}
  size_t GetFrame() const final { return 0; }
  size_t GetNumFrames() const final { return 0; }
  void SetColor(const base::Vector4f& color) final { nebula_color_ = color; }
  base::Vector4f GetColor() const final { return nebula_color_; }

  // Drawable interface.
  void Draw(float frame_frac) final;

  void SwitchColor(const base::Vector4f& color);

  void SetSpeed(float speed);

  const base::Vector4f& nebula_color() { return nebula_color_; }

 private:
  eng::Shader* shader_;

  base::Vector2f sky_offset_ = {0, 0};
  base::Vector2f last_sky_offset_ = {0, 0};
  base::Vector4f nebula_color_ = {0, 0, 0, 1};
  base::Vector2f scale_ = {1, 1};

  eng::Animator color_animator_;

  float speed_ = 0;

  bool without_nebula_ = false;
};

#endif  // DEMO_SKY_QUAD_H
