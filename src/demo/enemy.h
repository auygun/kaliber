#ifndef ENEMY_H
#define ENEMY_H

#include <array>
#include <list>
#include <memory>

#include "../base/vecmath.h"
#include "../engine/animator.h"
#include "../engine/sound_player.h"
#include "../engine/image_quad.h"
#include "../engine/solid_quad.h"
#include "damage_type.h"

namespace eng {
class Image;
class Sound;
class Texture;
}  // namespace eng

class Enemy {
 public:
  Enemy();
  ~Enemy();

  bool Initialize();

  void ContextLost();

  void Update(float delta_time);

  void Draw(float frame_frac);

  bool HasTarget(DamageType damage_type);
  base::Vector2 GetTargetPos(DamageType damage_type);

  void SelectTarget(DamageType damage_type,
                    const base::Vector2& origin,
                    const base::Vector2& dir);
  void DeselectTarget(DamageType damage_type);

  void HitTarget(DamageType damage_type);

  bool IsBossAlive() const;

  void PauseProgress();
  void ResumeProgress();

  void OnWaveStarted(int wave, bool boss_figt);

  void StopAllEnemyUnits();
  void KillAllEnemyUnits();
  void RemoveAll();

  int num_enemies_killed_in_current_wave() const {
    return num_enemies_killed_in_current_wave_;
  }

 private:
  struct EnemyUnit {
    EnemyType enemy_type = kEnemyType_Invalid;
    DamageType damage_type = kDamageType_Invalid;

    bool marked_for_removal = false;
    DamageType targetted_by_weapon_ = kDamageType_Invalid;
    int total_health = 0;
    int hit_points = 0;

    bool idle2_anim = false;
    bool stealth_active = false;

    bool shield_active = false;

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

    eng::SoundPlayer explosion;
    eng::SoundPlayer stealth;
    eng::SoundPlayer hit;
  };

  std::shared_ptr<eng::Texture> skull_tex_;
  std::shared_ptr<eng::Texture> bug_tex_;
  std::shared_ptr<eng::Texture> boss_tex_;
  std::shared_ptr<eng::Texture> target_tex_;
  std::shared_ptr<eng::Texture> blast_tex_;
  std::shared_ptr<eng::Texture> shield_tex_;
  std::shared_ptr<eng::Texture> score_tex_[kEnemyType_Max];

  eng::ImageQuad boss_;
  eng::Animator boss_animator_;
  eng::SoundPlayer boss_intro_;

  std::shared_ptr<eng::Sound> boss_intro_sound_;
  std::shared_ptr<eng::Sound> boss_explosion_sound_;
  std::shared_ptr<eng::Sound> explosion_sound_;
  std::shared_ptr<eng::Sound> stealth_sound_;
  std::shared_ptr<eng::Sound> hit_sound_;

  std::list<EnemyUnit> enemies_;

  int num_enemies_killed_in_current_wave_ = 0;

  std::array<float, kEnemyType_Unit_Last + 1> seconds_since_last_spawn_ =
      {0, 0, 0, 0};
  std::array<float, kEnemyType_Unit_Last + 1> seconds_to_next_spawn_ =
      {0, 0, 0, 0};

  float spawn_factor_ = 0;
  float spawn_factor_interpolator_ = 0;

  float boss_spawn_duration_ = 0;
  float boss_spawn_cooldown_ = 0;

  bool paused_ = true;

  int last_spawn_col_ = 0;

  bool boss_fight_ = false;

  void SpawnUnit(EnemyType enemy_type,
                 DamageType damage_type,
                 const base::Vector2& pos,
                 float speed);

  void SpawnBoss();

  void TakeDamage(EnemyUnit* target, int damage);

  void UpdateWave(float delta_time);
  void UpdateBoss(float delta_time);

  EnemyUnit* GetTarget(DamageType damage_type);

  int GetScore(EnemyType enemy_type);

  std::unique_ptr<eng::Image> GetScoreImage(int score);

  bool CreateRenderResources();

  void TranslateEnemyUnit(EnemyUnit& e, const base::Vector2& delta);
};

#endif  // ENEMY_H
