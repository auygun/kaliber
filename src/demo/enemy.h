#ifndef ENEMY_H
#define ENEMY_H

#include <array>
#include <list>
#include <memory>

#include "../base/vecmath.h"
#include "../engine/animator.h"
#include "../engine/image_quad.h"
#include "../engine/solid_quad.h"
#include "../engine/sound_player.h"
#include "damage_type.h"

namespace eng {
class Image;
class Sound;
}  // namespace eng

class Enemy {
 public:
  Enemy();
  ~Enemy();

  bool Initialize();

  void Update(float delta_time);

  void Pause(bool pause);

  bool HasTarget(DamageType damage_type);
  base::Vector2 GetTargetPos(DamageType damage_type);

  void SelectTarget(DamageType damage_type,
                    const base::Vector2& origin,
                    const base::Vector2& dir,
                    float snap_factor);
  void DeselectTarget(DamageType damage_type);

  void HitTarget(DamageType damage_type);

  void OnWaveFinished();
  void OnWaveStarted(int wave);

  int num_enemies_killed_in_current_wave() {
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

    eng::ImageQuad sprite;
    eng::ImageQuad target;
    eng::ImageQuad blast;
    eng::ImageQuad score;
    eng::SolidQuad health_base;
    eng::SolidQuad health_bar;

    eng::Animator movement_animator;
    eng::Animator sprite_animator;
    eng::Animator target_animator;
    eng::Animator blast_animator;
    eng::Animator health_animator;
    eng::Animator score_animator;

    eng::SoundPlayer explosion_;
  };

  std::shared_ptr<eng::Sound> explosion_sound_;

  std::list<EnemyUnit> enemies_;

  int num_enemies_killed_in_current_wave_ = 0;

  std::array<float, kEnemyType_Max> seconds_since_last_spawn_ = {0, 0, 0};
  std::array<float, kEnemyType_Max> seconds_to_next_spawn_ = {0, 0, 0};

  float spawn_factor_ = 0;
  float spawn_factor_interpolator_ = 0;

  bool waiting_for_next_wave_ = false;

  int last_spawn_col_ = 0;

  bool paused_ = false;

  void TakeDamage(EnemyUnit* target, int damage);

  void SpawnNextEnemy();

  void Spawn(EnemyType enemy_type,
             DamageType damage_type,
             const base::Vector2& pos,
             float speed);

  EnemyUnit* GetTarget(DamageType damage_type);

  int GetScore(EnemyType enemy_type);

  std::unique_ptr<eng::Image> GetScoreImage(EnemyType enemy_type);

  bool CreateRenderResources();
};

#endif  // ENEMY_H
