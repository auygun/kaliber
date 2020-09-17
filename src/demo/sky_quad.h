#ifndef SKY_QUAD_H
#define SKY_QUAD_H

#include "../base/vecmath.h"
#include "../engine/animatable.h"
#include "../engine/animator.h"

#include <array>
#include <memory>
#include <string>
#include <vector>

namespace eng {
class Shader;
}  // namespace eng

class SkyQuad : public eng::Animatable {
 public:
  SkyQuad();
  ~SkyQuad();

  SkyQuad(const SkyQuad&) = delete;
  SkyQuad& operator=(const SkyQuad&) = delete;

  bool Create();

  void Update(float delta_time);

  // Animatable interface.
  void SetFrame(size_t frame) override {}
  size_t GetFrame() const override { return 0; }
  size_t GetNumFrames() const override { return 0; }
  void SetColor(const base::Vector4& color) override { nebula_color_ = color; }
  base::Vector4 GetColor() const override { return nebula_color_; }

  // Drawable interface.
  void Draw(float frame_frac) override;

  void ContextLost();

  void SwitchColor(const base::Vector4& color);

  void SetSpeed(float speed);

 private:
  std::unique_ptr<eng::Shader> shader_;

  base::Vector2 sky_offset_ = {0, 0};
  base::Vector2 last_sky_offset_ = {0, 0};
  base::Vector4 nebula_color_ = {0, 0, 0, 1};
  base::Vector2 scale_ = {1, 1};

  eng::Animator color_animator_;

  float speed_ = 0;
};

#endif  // SKY_QUAD_H
