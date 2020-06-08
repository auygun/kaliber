#ifndef BOSS_H
#define BOSS_H

// #include <array>
// #include <list>
// #include <memory>

#include "../base/vecmath.h"
#include "../engine/animator.h"
#include "../engine/image_quad.h"
#include "../engine/solid_quad.h"
#include "damage_type.h"

namespace eng {
class Image;
// class Font;
class Texture;
}  // namespace eng

class Boss {
 public:
  Boss();
  ~Boss();

  bool Initialize();

  void ContextLost();

  void Update(float delta_time);

  void Draw(float frame_frac);

  void Hit(DamageType damage_type);

 private:
  // int total_health = 0;
  // int hit_points = 0;

  eng::ImageQuad sprite_;
  // eng::SolidQuad health_base;
  // eng::SolidQuad health_bar;

  eng::Animator sprite_animator_;

  std::shared_ptr<eng::Texture> boss_tex_;

  bool CreateRenderResources();
};

#endif  // BOSS_H
