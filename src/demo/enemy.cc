#include "enemy.h"

#include <algorithm>
#include <cassert>
#include <functional>
#include <limits>
#include <tuple>
#include <vector>

#include "../base/collusion_test.h"
#include "../base/interpolation.h"
#include "../base/log.h"
#include "../engine/engine.h"
#include "../engine/font.h"
#include "../engine/image.h"
#include "../engine/renderer/texture.h"
#include "demo.h"

using namespace base;
using namespace eng;

namespace {

constexpr int idle1_frame_start[][3] = {{0, 50, -1},
                                        {23, 73, -1},
                                        {-1, -1, 100},
                                        {13, 33, -1},
                                        {-1, -1, -1}};
constexpr int idle2_frame_start[][3] = {{7, 57, -1},
                                        {30, 80, -1},
                                        {-1, -1, 107},
                                        {-1, -1, -1}};

constexpr int idle1_frame_count[][3] = {{7, 7, -1},
                                        {7, 7, -1},
                                        {-1, -1, 7},
                                        {6, 6, -1}};
constexpr int idle2_frame_count[][3] = {{16, 16, -1},
                                        {16, 16, -1},
                                        {-1, -1, 16},
                                        {-1, -1, -1}};

constexpr int idle_frame_speed = 12;

constexpr int enemy_scores[] = {100, 150, 300, 200, 500};

// Enemy units spawn speed for waves.
constexpr float kSpawnPeriodWave[kEnemyType_Unit_Last + 1][2] = {{2, 5},
                                                                {15, 25},
                                                                {110, 130},
                                                                {70, 100}};

// Enemy units (adds) spawn speed for boss.
constexpr float kSpawnPeriodBoss[kEnemyType_Unit_Last + 1][2] = {{2, 5},
                                                                {15, 25},
                                                                {110, 130},
                                                                {70, 100}};

void SetupFadeOutAnim(Animator& animator, float delay) {
  animator.SetEndCallback(Animator::kTimer, [&]() -> void {
    animator.SetBlending({1, 1, 1, 0}, 0.5f,
                         std::bind(Acceleration, std::placeholders::_1, -1));
    animator.Play(Animator::kBlending, false);
  });
  animator.SetEndCallback(Animator::kBlending,
                          [&]() -> void { animator.SetVisible(false); });
  animator.SetTimer(delay);
}

float SnapSpawnPosX(int col) {
  Vector2 s = eng::Engine::Get().GetScreenSize();
  return (s.x / 4) / 2 + (s.x / 4) * col - s.x / 2;
}

}  // namespace

Enemy::Enemy()
    : skull_tex_(Engine::Get().CreateRenderResource<Texture>()),
      bug_tex_(Engine::Get().CreateRenderResource<Texture>()),
      boss_tex_(Engine::Get().CreateRenderResource<Texture>()),
      target_tex_(Engine::Get().CreateRenderResource<Texture>()),
      blast_tex_(Engine::Get().CreateRenderResource<Texture>()),
      score_tex_{Engine::Get().CreateRenderResource<Texture>(),
                 Engine::Get().CreateRenderResource<Texture>(),
                 Engine::Get().CreateRenderResource<Texture>(),
                 Engine::Get().CreateRenderResource<Texture>(),
                 Engine::Get().CreateRenderResource<Texture>()} {}

Enemy::~Enemy() = default;

bool Enemy::Initialize() {
  font_ = Engine::Get().GetAsset<Font>("PixelCaps!.ttf");
  if (!font_)
    return false;

  if (!CreateRenderResources())
    return false;

  boss_.Create(boss_tex_, {4, 3});
  boss_.AutoScale();
  boss_animator_.Attach(&boss_);

  return true;
}

void Enemy::ContextLost() {
  CreateRenderResources();
}

void Enemy::Update(float delta_time) {
  if (!waiting_for_next_wave_) {
    if (boss_fight_)
      UpdateBoss(delta_time);
    else
      UpdateWave(delta_time);
  }

  boss_animator_.Update(delta_time);

  Random& rnd = Engine::Get().GetRandomGenerator();

  // Update enemy units.
  for (auto it = enemies_.begin(); it != enemies_.end(); ++it) {
    if (it->marked_for_removal) {
      it = enemies_.erase(it);
      continue;
    }

    // Play random idle animation.
    if ((it->enemy_type == kEnemyType_LightSkull ||
         it->enemy_type == kEnemyType_DarkSkull ||
         it->enemy_type == kEnemyType_Tank) &&
        !it->idle2_anim && rnd.Roll(200) == 1) {
      it->idle2_anim = true;
      it->sprite_animator.Stop(Animator::kFrames);
      it->sprite.SetFrame(idle2_frame_start[it->enemy_type][it->damage_type]);
      it->sprite_animator.SetFrames(
          idle2_frame_count[it->enemy_type][it->damage_type], idle_frame_speed);
      auto& e = *it;
      it->sprite_animator.SetEndCallback(Animator::kFrames, [&]() -> void {
        e.sprite_animator.Stop(Animator::kFrames);
        e.sprite.SetFrame(idle1_frame_start[e.enemy_type][e.damage_type]);
        e.sprite_animator.SetFrames(
            idle1_frame_count[e.enemy_type][e.damage_type], idle_frame_speed);
        e.sprite_animator.Play(Animator::kFrames, true);
      });
      it->sprite_animator.Play(Animator::kFrames, false);
    }

    it->sprite_animator.Update(delta_time);
    it->target_animator.Update(delta_time);
    it->blast_animator.Update(delta_time);
    it->health_animator.Update(delta_time);
    it->score_animator.Update(delta_time);
    it->movement_animator.Update(delta_time);
  }
}

void Enemy::Draw(float frame_frac) {
  boss_.Draw();
  for (auto& e : enemies_) {
    e.sprite.Draw();
    e.target.Draw();
    e.blast.Draw();
    e.health_base.Draw();
    e.health_bar.Draw();
    e.score.Draw();
  }
}

bool Enemy::HasTarget(DamageType damage_type) {
  assert(damage_type > kDamageType_Invalid && damage_type < kDamageType_Any);

  return GetTarget(damage_type) ? true : false;
}

Vector2 Enemy::GetTargetPos(DamageType damage_type) {
  assert(damage_type > kDamageType_Invalid && damage_type < kDamageType_Any);

  EnemyUnit* target = GetTarget(damage_type);
  if (target)
    return target->sprite.GetOffset() -
           Vector2(0, target->sprite.GetScale().y / 2.5f);
  return {0, 0};
}

void Enemy::SelectTarget(DamageType damage_type,
                         const Vector2& origin,
                         const Vector2& dir) {
  assert(damage_type > kDamageType_Invalid && damage_type < kDamageType_Any);

  if (waiting_for_next_wave_)
    return;

  std::vector<std::tuple<EnemyUnit*, float, float>> candidates;

  for (auto& e : enemies_) {
    if (e.hit_points <= 0 || e.marked_for_removal || e.stealth)
      continue;

    if (e.targetted_by_weapon_ == damage_type) {
      e.targetted_by_weapon_ = kDamageType_Invalid;
      e.target.SetVisible(false);
      e.target_animator.Stop(Animator::kAllAnimations);
    }

    Vector2 weapon_enemy_dir = e.sprite.GetOffset() - origin;
    float enemy_weapon_dist = weapon_enemy_dir.Magnitude();
    weapon_enemy_dir.Normalize();
    float cos_theta = weapon_enemy_dir.DotProduct(dir);
    if (cos_theta > 0.95f)
      candidates.push_back(std::make_tuple(&e, cos_theta, enemy_weapon_dist));
  }

  if (candidates.empty())
    return;

  for (auto it = candidates.begin(); it != candidates.end();) {
    auto [cand_enemy, cand_cos_theta, cand_dist] = *it;

    auto oit = candidates.begin();
    for (; oit != candidates.end(); ++oit) {
      auto [other_enemy, other_cos_theta, other_dist] = *oit;

      if (cand_enemy == other_enemy || cand_dist < other_dist)
        continue;

      if (base::Intersection(other_enemy->sprite.GetOffset(),
                              other_enemy->sprite.GetScale() * 1.2f,
                              origin, dir)) {
        break;
      }
    }
    if (oit != candidates.end())
      it = candidates.erase(it);
    else
      ++it;
  }

  if (candidates.empty())
    return;

  std::sort(candidates.begin(), candidates.end(), [](auto& a, auto& b) {
    return std::get<1>(a) > std::get<1>(b);
  });

  EnemyUnit* best_enemy = nullptr;
  for (auto& cand : candidates) {
    if (std::get<0>(cand)->damage_type == damage_type ||
        std::get<0>(cand)->damage_type == kDamageType_Any) {
      best_enemy = std::get<0>(cand);
      break;
    }
  }
  if (!best_enemy)
    best_enemy = std::get<0>(candidates[0]);

  if (best_enemy) {
    best_enemy->targetted_by_weapon_ = damage_type;
    best_enemy->target.SetVisible(true);
    if (damage_type == kDamageType_Green) {
      best_enemy->target.SetFrame(0);
      best_enemy->target_animator.SetFrames(6, 28);
    } else {
      best_enemy->target.SetFrame(6);
      best_enemy->target_animator.SetFrames(6, 28);
    }
    best_enemy->target_animator.Play(Animator::kFrames, false);
  }
}

void Enemy::DeselectTarget(DamageType damage_type) {
  assert(damage_type > kDamageType_Invalid && damage_type < kDamageType_Any);

  EnemyUnit* target = GetTarget(damage_type);
  if (target) {
    target->targetted_by_weapon_ = kDamageType_Invalid;
    target->target.SetVisible(false);
    target->target_animator.Stop(Animator::kAllAnimations);
  }
}

void Enemy::HitTarget(DamageType damage_type) {
  assert(damage_type > kDamageType_Invalid && damage_type < kDamageType_Any);

  if (waiting_for_next_wave_)
    return;

  EnemyUnit* target = GetTarget(damage_type);

  if (!target)
    return;

  target->target.SetVisible(false);
  target->target_animator.Stop(Animator::kAllAnimations);

  if ((target->damage_type != kDamageType_Any &&
       target->damage_type != damage_type))
    return;

  TakeDamage(target, 1);
}

bool Enemy::IsBossAlive() const {
  return boss_fight_ && boss_.GetFrame() < 9;
}

void Enemy::OnWaveFinished() {
  for (auto& e : enemies_) {
    if (!e.marked_for_removal && e.hit_points > 0)
      e.movement_animator.Pause(Animator::kMovement);
    if (e.stealth) {
      e.sprite_animator.Stop(Animator::kAllAnimations);
      e.sprite_animator.SetEndCallback(Animator::kBlending, nullptr);
      e.sprite_animator.SetBlending({1, 1, 1, 1}, 0.5f);
      e.sprite_animator.Play(Animator::kBlending, false);
      e.sprite_animator.Play(Animator::kFrames, true);
    }
  }
  waiting_for_next_wave_ = true;
}

void Enemy::KillAllEnemyUnits() {
  for (auto& e : enemies_) {
    if (!e.marked_for_removal && e.hit_points > 0)
      TakeDamage(&e, 100);
  }
}

void Enemy::OnWaveStarted(int wave, bool boss_fight) {
  num_enemies_killed_in_current_wave_ = 0;
  seconds_since_last_spawn_ = {0, 0, 0, 0};
  seconds_to_next_spawn_ = {0, 0, 0, 0};
  spawn_factor_ = 1 / (log10(0.25f * ((wave + 4) * 0.8f) + 1.468f) * 6);
  spawn_factor_interpolator_ = 0;
  boss_spawn_cooldown_ = 5;
  boss_spawn_duration_ = 0;
  last_spawn_col_ = 0;
  waiting_for_next_wave_ = false;
  boss_fight_ = boss_fight;

  if (boss_fight)
    SpawnBoss();
}

void Enemy::SpawnUnit(EnemyType enemy_type,
                      DamageType damage_type,
                      const Vector2& pos,
                      float speed) {
  assert(enemy_type > kEnemyType_Invalid && enemy_type < kEnemyType_Unit_Last + 1);
  assert(damage_type > kDamageType_Invalid && damage_type < kDamageType_Max);

  Engine& engine = Engine::Get();
  Demo* game = static_cast<Demo*>(engine.GetGame());

  auto& e = enemies_.emplace_back();
  e.enemy_type = enemy_type;
  e.damage_type = damage_type;
  switch (enemy_type) {
    case kEnemyType_LightSkull:
      e.total_health = e.hit_points = 1;
      e.sprite.Create(skull_tex_, {10, 13}, 100, 100);
      break;
    case kEnemyType_DarkSkull:
      e.total_health = e.hit_points = 2;
      e.sprite.Create(skull_tex_, {10, 13}, 100, 100);
      break;
    case kEnemyType_Tank:
      e.total_health = e.hit_points = 6;
      e.sprite.Create(skull_tex_, {10, 13}, 100, 100);
      break;
    case kEnemyType_Bug:
      e.total_health = e.hit_points = 2;
      e.sprite.Create(bug_tex_, {10, 4});
      break;
    default:
      assert(false);
  }
  e.sprite.AutoScale();
  e.sprite.SetVisible(true);
  Vector2 spawn_pos = pos + Vector2(0, e.sprite.GetScale().y / 2);
  e.sprite.SetOffset(spawn_pos);

  e.sprite.SetFrame(idle1_frame_start[enemy_type][damage_type]);
  e.sprite_animator.SetFrames(idle1_frame_count[enemy_type][damage_type],
                              idle_frame_speed);

  e.sprite_animator.Attach(&e.sprite);
  e.sprite_animator.Play(Animator::kFrames, true);

  e.target.Create(target_tex_, {6, 2});
  e.target.AutoScale();
  e.target.SetOffset(spawn_pos);

  e.blast.Create(blast_tex_, {6, 2});
  e.blast.AutoScale();
  e.blast.SetOffset(spawn_pos);

  e.health_base.Scale(e.sprite.GetScale() * Vector2(0.6f, 0.01f));
  e.health_base.SetOffset(spawn_pos);
  e.health_base.PlaceToBottomOf(e.sprite);
  e.health_base.SetColor({0.5f, 0.5f, 0.5f, 1});

  e.health_bar.Scale(e.sprite.GetScale() * Vector2(0.6f, 0.01f));
  e.health_bar.SetOffset(spawn_pos);
  e.health_bar.PlaceToBottomOf(e.sprite);
  e.health_bar.SetColor({0.161f, 0.89f, 0.322f, 1});

  e.score.Create(score_tex_[e.enemy_type]);
  e.score.AutoScale();
  e.score.SetColor({1, 1, 1, 1});
  e.score.SetOffset(spawn_pos);

  e.target_animator.Attach(&e.target);

  e.blast_animator.SetEndCallback(Animator::kFrames,
                                  [&]() -> void { e.blast.SetVisible(false); });
  if (damage_type == kDamageType_Green) {
    e.blast.SetFrame(0);
    e.blast_animator.SetFrames(6, 28);
  } else {
    e.blast.SetFrame(6);
    e.blast_animator.SetFrames(6, 28);
  }
  e.blast_animator.Attach(&e.blast);

  SetupFadeOutAnim(e.health_animator, 1);
  e.health_animator.Attach(&e.health_base);
  e.health_animator.Attach(&e.health_bar);

  SetupFadeOutAnim(e.score_animator, 0.2f);
  e.score_animator.SetMovement({0, engine.GetScreenSize().y / 2}, 2.0f);
  e.score_animator.SetEndCallback(
      Animator::kMovement, [&]() -> void { e.marked_for_removal = true; });
  e.score_animator.Attach(&e.score);

  float max_distance =
      engine.GetScreenSize().y - game->GetPlayer().GetWeaponScale().y / 2;

  Animator::Interpolator interpolator;
  if (boss_fight_)
    interpolator = std::bind(CatmullRom, std::placeholders::_1, 2.5f, 1.2f);
  else
    interpolator = std::bind(Acceleration, std::placeholders::_1, -0.15f);
  e.movement_animator.SetMovement({0, -max_distance}, speed, interpolator);
  e.movement_animator.SetEndCallback(Animator::kMovement, [&]() -> void {
    e.sprite.SetVisible(false);
    e.target.SetVisible(false);
    e.blast.SetVisible(false);
    e.marked_for_removal = true;
  });
  e.movement_animator.Attach(&e.sprite);
  e.movement_animator.Attach(&e.target);
  e.movement_animator.Attach(&e.blast);
  e.movement_animator.Attach(&e.health_base);
  e.movement_animator.Attach(&e.health_bar);
  e.movement_animator.Attach(&e.score);
  e.movement_animator.Play(Animator::kMovement, false);
}

void Enemy::SpawnBoss() {
  boss_.SetVisible(true);

  boss_.SetOffset(Engine::Get().GetScreenSize() * Vector2(0, 0.5f) +
                  boss_.GetScale() * Vector2(0, 2.0f));
  boss_animator_.SetMovement({0, boss_.GetScale().y * -2.4f}, 5,
      std::bind(Acceleration, std::placeholders::_1, -1));
  boss_.SetFrame(0);
  boss_animator_.SetFrames(8, 12);

  boss_animator_.SetEndCallback(Animator::kMovement, [&]() -> void {
    auto& e = enemies_.emplace_front();
    e.enemy_type = kEnemyType_Boss;
    e.damage_type = kDamageType_Any;
    e.total_health = e.hit_points = 40;

    Vector2 hit_box_pos = boss_.GetOffset() - boss_.GetScale() * Vector2(0, 0.2f);

    // Just a hit box, not the visual sprite of the boss.
    e.sprite.SetOffset(hit_box_pos);
    e.sprite.SetScale(boss_.GetScale() * 0.3f);

    e.target.Create(target_tex_, {6, 2});
    e.target.AutoScale();
    e.target.SetOffset(hit_box_pos);

    Vector2 health_bar_offset = boss_.GetScale() * Vector2(0, 0.2f);

    e.health_base.Scale(e.sprite.GetScale() * Vector2(0.7f, 0.08f));
    e.health_base.SetOffset(hit_box_pos + health_bar_offset);
    e.health_base.PlaceToBottomOf(boss_);
    e.health_base.SetColor({0.5f, 0.5f, 0.5f, 1});
    e.health_base.SetVisible(true);

    e.health_bar.Scale(e.sprite.GetScale() * Vector2(0.7f, 0.08f));
    e.health_bar.SetOffset(hit_box_pos + health_bar_offset);
    e.health_bar.PlaceToBottomOf(boss_);
    e.health_bar.SetColor({0.161f, 0.89f, 0.322f, 1});
    e.health_bar.SetVisible(true);

    e.score.Create(score_tex_[e.enemy_type]);
    e.score.AutoScale();
    e.score.SetColor({1, 1, 1, 1});
    e.score.SetOffset(hit_box_pos);

    e.target_animator.Attach(&e.target);

    SetupFadeOutAnim(e.score_animator, 0.2f);
    e.score_animator.SetMovement({0, Engine::Get().GetScreenSize().y / 2},
                                 2.0f);
    e.score_animator.SetEndCallback(
        Animator::kMovement, [&]() -> void { e.marked_for_removal = true; });
    e.score_animator.Attach(&e.score);
  });
  boss_animator_.Play(Animator::kFrames, true);
  boss_animator_.Play(Animator::kMovement, false);
}

void Enemy::TakeDamage(EnemyUnit* target, int damage) {
  assert(!target->marked_for_removal);
  assert(target->hit_points > 0);

  target->blast.SetVisible(true);
  target->blast_animator.Play(Animator::kFrames, false);

  target->hit_points -= damage;
  if (target->hit_points <= 0) {
    if (!waiting_for_next_wave_)
      ++num_enemies_killed_in_current_wave_;

    target->sprite.SetVisible(false);
    target->health_base.SetVisible(false);
    target->health_bar.SetVisible(false);
    target->score.SetVisible(true);

    target->score_animator.Play(Animator::kTimer | Animator::kMovement, false);
    target->movement_animator.Pause(Animator::kMovement);

    Engine& engine = Engine::Get();
    Demo* game = static_cast<Demo*>(engine.GetGame());
    game->AddScore(GetScore(target->enemy_type));

    if (target->enemy_type == kEnemyType_Boss) {
      boss_animator_.SetEndCallback(Animator::kMovement, [&]() -> void {
        boss_animator_.SetVisible(false);
      });
      boss_animator_.SetMovement({0, boss_.GetScale().y * 0.99f}, 4);
      boss_.SetFrame(9);
      boss_animator_.SetFrames(2, 12);
      boss_animator_.SetEndCallback(Animator::kTimer, [&]() -> void {
        boss_animator_.Stop(Animator::kFrames);
        boss_animator_.Play(Animator::kMovement, false);
        boss_.SetFrame(11);
      });
      boss_animator_.SetTimer(1);
      boss_animator_.Play(Animator::kFrames | Animator::kTimer, true);
    }
  } else {
    target->targetted_by_weapon_ = kDamageType_Invalid;

    Vector2 s = target->health_base.GetScale();
    s.x *= (float)target->hit_points / (float)target->total_health;
    float t = (s.x - target->health_bar.GetScale().x) / 2;
    target->health_bar.SetScale(s);
    target->health_bar.Translate({t, 0});

    target->health_base.SetVisible(true);
    target->health_bar.SetVisible(true);

    target->health_animator.Stop(Animator::kTimer | Animator::kBlending);
    target->health_animator.Play(Animator::kTimer, false);

    if (target->enemy_type == kEnemyType_Bug) {
      target->stealth = true;
      target->movement_animator.Pause(Animator::kMovement);
      target->sprite_animator.Pause(Animator::kFrames);

      Random& rnd = Engine::Get().GetRandomGenerator();
      float stealth_timer = Lerp(2.0f, 5.0f, rnd.GetFloat());
      target->sprite_animator.SetEndCallback(Animator::kTimer,
          [&, target]() -> void {
            float x = SnapSpawnPosX(rnd.Roll(4) - 1);
            TranslateEnemyUnit(*target, {x - target->sprite.GetOffset().x,0});

            target->sprite_animator.SetEndCallback(Animator::kBlending,
                [&, target]() -> void {
                  target->stealth = false;
                  target->movement_animator.Play(Animator::kMovement, true);
                  target->sprite_animator.Play(Animator::kFrames, false);
                });
            target->sprite_animator.SetBlending({1, 1, 1, 1}, 1.0f);
            target->sprite_animator.Play(Animator::kBlending, false);
          });

      target->sprite_animator.SetTimer(stealth_timer);
      target->sprite_animator.SetBlending({1, 1, 1, 0}, 1.5f);
      target->sprite_animator.Play(Animator::kBlending | Animator::kTimer,
                                   false);
    }
  }
}

void Enemy::UpdateWave(float delta_time) {
  if (spawn_factor_interpolator_ < 1) {
    spawn_factor_interpolator_ += delta_time * 0.1f;
    if (spawn_factor_interpolator_ > 1)
      spawn_factor_interpolator_ = 1;
  }

  for (int i = 0; i < kEnemyType_Unit_Last + 1; ++i)
    seconds_since_last_spawn_[i] += delta_time;

  Engine& engine = Engine::Get();
  Random& rnd = engine.GetRandomGenerator();

  float factor = Lerp(1.0f, spawn_factor_, spawn_factor_interpolator_);
  EnemyType enemy_type = kEnemyType_Invalid;

  for (int i = 0; i < kEnemyType_Unit_Last + 1; ++i) {
    if (seconds_since_last_spawn_[i] >= seconds_to_next_spawn_[i]) {
      if (seconds_to_next_spawn_[i] > 0)
        enemy_type = (EnemyType)i;

      seconds_since_last_spawn_[i] = 0;
      seconds_to_next_spawn_[i] =
          Lerp(kSpawnPeriodWave[i][0] * factor,
               kSpawnPeriodWave[i][1] * factor,
               rnd.GetFloat());
      break;
    }
  }

  if (enemy_type == kEnemyType_Invalid)
    return;

  DamageType damage_type = enemy_type == kEnemyType_Tank
                               ? kDamageType_Any
                               : (DamageType)(rnd.Roll(2) - 1);

  Vector2 s = engine.GetScreenSize();
  int col;
  col = rnd.Roll(4) - 1;
  if (col == last_spawn_col_)
    col = (col + 1) % 4;
  last_spawn_col_ = col;
  float x = SnapSpawnPosX(col);
  Vector2 pos = {x, s.y / 2};
  float speed = enemy_type == kEnemyType_Tank
                    ? 36.0f
                    : (rnd.Roll(4) == 4 ? 6.0f : 10.0f);

  SpawnUnit(enemy_type, damage_type, pos, speed);
}

void Enemy::UpdateBoss(float delta_time) {
  if (boss_spawn_cooldown_ > 0) {
    boss_spawn_cooldown_ -= delta_time;
    if (boss_spawn_cooldown_ > 0)
      return;
    boss_spawn_duration_ = 6;
  }

  boss_spawn_duration_ -= delta_time;
  if (boss_spawn_duration_ <= 0) {
    boss_spawn_cooldown_ = 5;
    return;
  }

  for (int i = 0; i < kEnemyType_Unit_Last + 1; ++i)
    seconds_since_last_spawn_[i] += delta_time;

  Engine& engine = Engine::Get();
  Random& rnd = engine.GetRandomGenerator();

  EnemyType enemy_type = kEnemyType_Invalid;

  for (int i = 0; i < kEnemyType_Unit_Last + 1; ++i) {
    if (seconds_since_last_spawn_[i] >= seconds_to_next_spawn_[i]) {
      if (seconds_to_next_spawn_[i] > 0)
        enemy_type = (EnemyType)i;

      seconds_since_last_spawn_[i] = 0;
      seconds_to_next_spawn_[i] =
          Lerp(kSpawnPeriodBoss[i][0] * spawn_factor_ * 0.8f,
               kSpawnPeriodBoss[i][1] * spawn_factor_ * 0.8f,
               rnd.GetFloat());
      break;
    }
  }

  if (enemy_type == kEnemyType_Invalid)
    return;

  DamageType damage_type = enemy_type == kEnemyType_Tank
                               ? kDamageType_Any
                               : (DamageType)(rnd.Roll(2) - 1);

  int col = (last_spawn_col_++) % 2;
  float offset = Lerp(boss_.GetScale().x * -0.12f, boss_.GetScale().x * 0.12f, rnd.GetFloat());
  float x = (boss_.GetScale().x / 3) * (col ? 1 : -1) + offset;
  Vector2 pos = {x, boss_.GetOffset().y - boss_.GetScale().y / 2};
  float speed = enemy_type == kEnemyType_Tank
                    ? 36.0f
                    : (rnd.Roll(4) == 4 ? 6.0f : 10.0f);

  SpawnUnit(enemy_type, damage_type, pos, speed);
}

Enemy::EnemyUnit* Enemy::GetTarget(DamageType damage_type) {
  for (auto& e : enemies_) {
    if (e.targetted_by_weapon_ == damage_type && e.hit_points > 0 &&
        !e.marked_for_removal)
      return &e;
  }
  return nullptr;
}

int Enemy::GetScore(EnemyType enemy_type) {
  assert(enemy_type > kEnemyType_Invalid && enemy_type < kEnemyType_Max);
  return enemy_scores[enemy_type];
}

std::shared_ptr<Image> Enemy::GetScoreImage(int score) {
  std::string text = std::to_string(score);
  int width, height;
  font_->CalculateBoundingBox(text.c_str(), width, height);

  auto image = std::make_shared<Image>();
  image->Create(width, height);
  image->Clear({1, 1, 1, 0});

  font_->Print(0, 0, text.c_str(), image->GetBuffer(), image->GetWidth());

  image->Compress();
  image->SetImmutable();
  return image;
}

bool Enemy::CreateRenderResources() {
  Engine& engine = Engine::Get();

  auto skull_image = engine.GetAsset<Image>("enemy_anims_01_frames_ok.png");
  auto bug_image = engine.GetAsset<Image>("enemy_anims_02_frames_ok.png");
  auto boss_image = engine.GetAsset<Image>("Boss_ok.png");
  auto target_image = engine.GetAsset<Image>("enemy_target_single_ok.png");
  auto blast_image = engine.GetAsset<Image>("enemy_anims_blast_ok.png");
  if (!skull_image || !bug_image || !boss_image || !target_image ||
      !blast_image)
    return false;

  skull_tex_->Update(skull_image);
  bug_tex_->Update(bug_image);
  boss_tex_->Update(boss_image);
  target_tex_->Update(target_image);
  blast_tex_->Update(blast_image);

  for (int i = 0; i < kEnemyType_Max; ++i)
    score_tex_[i]->Update(GetScoreImage(GetScore((EnemyType)i)));

  return true;
}

void Enemy::TranslateEnemyUnit(EnemyUnit& e, const Vector2& delta) {
  e.sprite.Translate(delta);
  e.target.Translate(delta);
  e.blast.Translate(delta);
  e.health_base.Translate(delta);
  e.health_bar.Translate(delta);
  e.score.Translate(delta);
}
