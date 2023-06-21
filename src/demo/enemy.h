#ifndef DEMO_ENEMY_H
#define DEMO_ENEMY_H

#include <array>
#include <list>
#include <memory>

#include "base/vecmath.h"
#include "engine/animator.h"
#include "engine/image_quad.h"
#include "engine/solid_quad.h"
#include "engine/sound_player.h"

#include "demo/damage_type.h"

namespace eng {
class Image;
}  // namespace eng

class Enemy {
 public:
  Enemy();
  ~Enemy();

  bool PreInitialize();
  bool Initialize();

  void Update(float delta_time);

  void Pause(bool pause);

  bool HasTarget(DamageType damage_type);
  base::Vector2f GetTargetPos(DamageType damage_type);

  void SelectTarget(DamageType damage_type,
                    const base::Vector2f& origin,
                    const base::Vector2f& dir);
  void DeselectTarget(DamageType damage_type);

  void HitTarget(DamageType damage_type);

  bool IsBossAlive() const;

  void PauseProgress();
  void ResumeProgress();

  void OnWaveStarted(int wave, bool boss_figt);

  void StopAllEnemyUnits(bool chromatic_aberration_effect = false);
  void KillAllEnemyUnits(bool randomize_order = true);
  void RemoveAll();

  void KillBoss();

  void Reset();

  int num_enemies_killed_in_current_wave() const {
    return num_enemies_killed_in_current_wave_;
  }

 private:
  struct EnemyUnit {
    EnemyType enemy_type = kEnemyType_Invalid;
    DamageType damage_type = kDamageType_Invalid;
    SpeedType speed_type = kSpeedType_Invalid;

    bool marked_for_removal = false;
    bool targetted_by_weapon_[2] = {false, false};
    int total_health = 0;
    int hit_points = 0;

    float kill_timer = 0;

    bool idle2_anim = false;
    bool stealth_active = false;

    bool shield_active = false;

    bool freeze_ = false;

    bool chromatic_aberration_active_ = false;

    eng::ImageQuad sprite;
    eng::ImageQuad target;
    eng::ImageQuad blast;
    eng::ImageQuad shield;
    eng::ImageQuad score;
    eng::SolidQuad health_base;
    eng::SolidQuad health_bar;

    eng::Animator movement_animator;
    eng::Animator sprite_animator;
    eng::Animator target_animator;
    eng::Animator blast_animator;
    eng::Animator shield_animator;
    eng::Animator health_animator;
    eng::Animator score_animator;

    eng::SoundPlayer spawn;
    eng::SoundPlayer explosion;
    eng::SoundPlayer stealth;
    eng::SoundPlayer shield_on;
    eng::SoundPlayer hit;
  };

  float chromatic_aberration_offset_ = 0;

  eng::ImageQuad boss_;
  eng::Animator boss_animator_;
  eng::SoundPlayer boss_intro_;

  std::list<EnemyUnit> enemies_;

  int num_enemies_killed_in_current_wave_ = 0;

  std::array<float, kEnemyType_Unit_Last + 1> seconds_since_last_spawn_ = {
      0, 0, 0, 0};
  std::array<float, kEnemyType_Unit_Last + 1> seconds_to_next_spawn_ = {0, 0, 0,
                                                                        0};

  float spawn_factor_ = 0;

  float boss_spawn_time_ = 0;
  float boss_spawn_time_factor_ = 0;

  float seconds_since_last_power_up_ = 0;
  float seconds_to_next_power_up_ = 0;

  bool progress_paused_ = true;

  int last_spawn_col_ = 0;

  int wave_ = 0;
  bool boss_fight_ = false;

  bool CheckSpawnPos(base::Vector2f pos, SpeedType speed_type);
  bool CheckTeleportPos(EnemyUnit* enemy);

  void SpawnUnit(EnemyType enemy_type,
                 DamageType damage_type,
                 const base::Vector2f& pos,
                 float speed,
                 SpeedType speed_type = kSpeedType_Invalid);

  void SpawnBoss();

  void TakeDamage(EnemyUnit* target, int damage);

  void UpdateWave(float delta_time);
  void UpdateBoss(float delta_time);

  EnemyUnit* GetTarget(DamageType damage_type);

  int GetScore(EnemyType enemy_type);

  std::unique_ptr<eng::Image> GetScoreImage(EnemyType enemy_type);

  void TranslateEnemyUnit(EnemyUnit& e, const base::Vector2f& delta);
};

#endif  // DEMO_ENEMY_H
