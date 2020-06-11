#ifndef PLAYER_H
#define PLAYER_H

#include <memory>

#include "../base/vecmath.h"
#include "../engine/animator.h"
#include "../engine/image_quad.h"
#include "../engine/solid_quad.h"
#include "../engine/renderer/texture.h"
#include "damage_type.h"

namespace eng {
class Font;
class Image;
class InputEvent;
}  //  namespace eng

class Player {
 public:
  Player();
  ~Player();

  bool Initialize();

  void ContextLost();

  void Update(float delta_time);

  void OnInputEvent(std::unique_ptr<eng::InputEvent> event);

  void Draw(float frame_frac);

  void TakeDamage(int damage);

  void Reset();

  base::Vector2 GetWeaponPos(DamageType type) const;
  base::Vector2 GetWeaponScale() const;

 private:
  std::shared_ptr<eng::Texture> weapon_tex_;
  std::shared_ptr<eng::Texture> beam_tex_;

  eng::ImageQuad drag_sign_[2];
  eng::ImageQuad weapon_[2];
  eng::ImageQuad beam_[2];
  eng::ImageQuad beam_spark_[2];

  eng::Animator warmup_animator_[2];
  eng::Animator cooldown_animator_[2];
  eng::Animator beam_animator_[2];
  eng::Animator spark_animator_[2];

  eng::SolidQuad health_bar_[2];

  eng::SolidQuad nuke_;
  eng::Animator nuke_animator_;

  std::shared_ptr<eng::Texture> nuke_counter_tex_;
  eng::ImageQuad nuke_counter_;

  std::shared_ptr<const eng::Font> font_;

  int nuke_count_ = 0;

  int total_health_ = 3;
  int hit_points_ = 0;

  DamageType active_weapon_ = kDamageType_Invalid;

  base::Vector2 drag_start_ = {0, 0};
  base::Vector2 drag_end_ = {0, 0};
  bool drag_valid_ = false;

  DamageType GetWeaponType(const base::Vector2& pos);

  void SetBeamLength(DamageType type, float len);

  void WarmupWeapon(DamageType type);
  void CooldownWeapon(DamageType type);

  void Fire(DamageType type, base::Vector2 dir);
  bool IsFiring(DamageType type);

  void SetupWeapons();

  void UpdateTarget();

  void Nuke();

  void DragStart(const base::Vector2& pos);
  void Drag(const base::Vector2& pos);
  void DragEnd();
  void DragCancel();
  bool ValidateDrag();

  void NavigateBack();

  bool CreateRenderResources();

  std::shared_ptr<eng::Image> GetNukeCounterImage(int n);
};

#endif  // PLAYER_H
