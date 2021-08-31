#ifndef PLAYER_H
#define PLAYER_H

#include <memory>

#include "../base/vecmath.h"
#include "../engine/animator.h"
#include "../engine/image_quad.h"
#include "../engine/solid_quad.h"
#include "../engine/sound_player.h"
#include "damage_type.h"

namespace eng {
class InputEvent;
class Sound;
}  //  namespace eng

class Player {
 public:
  Player();
  ~Player();

  bool Initialize();

  void Update(float delta_time);

  void Pause(bool pause);

  void OnInputEvent(std::unique_ptr<eng::InputEvent> event);

  void TakeDamage(int damage);

  void AddNuke(int n);

  void Reset();

  base::Vector2f GetWeaponPos(DamageType type) const;
  base::Vector2f GetWeaponScale() const;

  int nuke_count() { return nuke_count_; }

 private:
  std::shared_ptr<eng::Sound> nuke_explosion_sound_;
  std::shared_ptr<eng::Sound> no_nuke_sound_;
  std::shared_ptr<eng::Sound> laser_shot_sound_;

  eng::ImageQuad drag_sign_[2];
  eng::ImageQuad weapon_[2];
  eng::ImageQuad beam_[2];
  eng::ImageQuad beam_spark_[2];

  eng::SoundPlayer laser_shot_[2];

  eng::Animator warmup_animator_[2];
  eng::Animator cooldown_animator_[2];
  eng::Animator beam_animator_[2];
  eng::Animator spark_animator_[2];

  eng::ImageQuad health_bead_[3];

  eng::SolidQuad nuke_;
  eng::Animator nuke_animator_;
  eng::SoundPlayer nuke_explosion_;
  eng::SoundPlayer no_nuke_;

  eng::ImageQuad nuke_symbol_;
  eng::Animator nuke_symbol_animator_;

  int nuke_count_ = 0;

  int total_health_ = 3;
  int hit_points_ = 0;

  base::Vector2f drag_start_[2] = {{0, 0}, {0, 0}};
  base::Vector2f drag_end_[2] = {{0, 0}, {0, 0}};
  DamageType drag_weapon_[2] = {kDamageType_Invalid, kDamageType_Invalid};
  bool drag_valid_[2] = {false, false};
  int weapon_drag_ind[2] = {0, 0};

  bool drag_nuke_[2] = {false, false};

  DamageType GetWeaponType(const base::Vector2f& pos);

  void WarmupWeapon(DamageType type);
  void CooldownWeapon(DamageType type);

  void Fire(DamageType type, base::Vector2f dir);
  bool IsFiring(DamageType type);

  void SetupWeapons();

  void UpdateTarget(DamageType weapon);

  void Nuke();

  void DragStart(int i, const base::Vector2f& pos);
  void Drag(int i, const base::Vector2f& pos);
  void DragEnd(int i);
  void DragCancel(int i);
  bool ValidateDrag(int i);

  void NavigateBack();

  bool CreateRenderResources();
};

#endif  // PLAYER_H
