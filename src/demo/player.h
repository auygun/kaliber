#ifndef PLAYER_H
#define PLAYER_H

#include <memory>

#include "../base/vecmath.h"
#include "../engine/animator.h"
#include "../engine/image_quad.h"
#include "../engine/solid_quad.h"
#include "../engine/sound_player.h"
#include "../engine/renderer/texture.h"
#include "damage_type.h"

namespace eng {
class Image;
class InputEvent;
class Sound;
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

  void AddNuke(int n);

  void Reset();

  base::Vector2 GetWeaponPos(DamageType type) const;
  base::Vector2 GetWeaponScale() const;

 private:
  std::shared_ptr<eng::Texture> weapon_tex_;
  std::shared_ptr<eng::Texture> beam_tex_;

  std::shared_ptr<eng::Sound> nuke_explosion_sound_;
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

  eng::SolidQuad health_bar_[2];

  eng::SolidQuad nuke_;
  eng::Animator nuke_animator_;
  eng::SoundPlayer nuke_explosion_;

  std::shared_ptr<eng::Texture> nuke_symbol_tex_;
  std::shared_ptr<eng::Texture> nuke_counter_tex_;
  eng::ImageQuad nuke_symbol_;
  eng::ImageQuad nuke_counter_;
  eng::Animator nuke_symbol_animator_;

  int nuke_count_ = 0;

  int total_health_ = 3;
  int hit_points_ = 0;

  base::Vector2 drag_start_[2] = {{0, 0}, {0, 0}};
  base::Vector2 drag_end_[2] = {{0, 0}, {0, 0}};
  DamageType drag_weapon_[2] = {kDamageType_Invalid, kDamageType_Invalid};
  bool drag_valid_[2] = {false, false};
  int weapon_drag_ind[2] = {0, 0};

  bool drag_nuke_[2] = {false, false};

  DamageType GetWeaponType(const base::Vector2& pos);

  void SetBeamLength(DamageType type, float len);

  void WarmupWeapon(DamageType type);
  void CooldownWeapon(DamageType type);

  void Fire(DamageType type, base::Vector2 dir);
  bool IsFiring(DamageType type);

  void SetupWeapons();

  void UpdateTarget(DamageType weapon);

  void Nuke();

  void DragStart(int i, const base::Vector2& pos);
  void Drag(int i, const base::Vector2& pos);
  void DragEnd(int i);
  void DragCancel(int i);
  bool ValidateDrag(int i);

  void NavigateBack();

  bool CreateRenderResources();

  std::unique_ptr<eng::Image> GetNukeCounterImage(int n);
};

#endif  // PLAYER_H
