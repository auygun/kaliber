#include "demo/enemy.h"

#include <algorithm>
#include <functional>
#include <limits>
#include <tuple>
#include <vector>

#include "base/collusion_test.h"
#include "base/interpolation.h"
#include "base/log.h"
#include "engine/engine.h"
#include "engine/font.h"
#include "engine/image.h"
#include "engine/renderer/geometry.h"
#include "engine/renderer/shader.h"
#include "engine/shader_source.h"
#include "engine/sound.h"

#include "demo/demo.h"

using namespace std::string_literals;

using namespace base;
using namespace eng;

namespace {

constexpr int idle1_frame_start[][3] = {{0, 50, -1},   {23, 73, -1},
                                        {-1, -1, 100}, {13, 33, -1},
                                        {-1, -1, 0},   {-1, -1, -1}};
constexpr int idle2_frame_start[][3] = {{7, 57, -1},
                                        {30, 80, -1},
                                        {-1, -1, 107},
                                        {-1, -1, -1},
                                        {-1, -1, -1}};

constexpr int idle1_frame_count[][3] = {{7, 7, -1},
                                        {7, 7, -1},
                                        {-1, -1, 7},
                                        {6, 6, -1},
                                        {0, 0, 8}};
constexpr int idle2_frame_count[][3] = {{16, 16, -1},
                                        {16, 16, -1},
                                        {-1, -1, 16},
                                        {-1, -1, -1},
                                        {-1, -1, -1}};

constexpr int idle_frame_speed = 12;

constexpr int enemy_scores[] = {100, 150, 300, 250, 0, 500};

// Enemy units spawn speed.
constexpr float kSpawnPeriod[kEnemyType_Unit_Last + 1][2] = {{3, 6},
                                                             {20, 30},
                                                             {60, 80},
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
  Vector2f s = eng::Engine::Get().GetScreenSize();
  float offset = base::Lerp(s.x * -0.02f, s.x * 0.02f,
                            eng::Engine::Get().GetRandomGenerator().GetFloat());
  return (s.x / 4) / 2 + (s.x / 4) * col - s.x / 2 + offset;
}

}  // namespace

Enemy::Enemy()
    : chromatic_aberration_(Engine::Get().CreateRenderResource<Shader>()) {}

Enemy::~Enemy() = default;

bool Enemy::Initialize() {
  boss_intro_sound_ = std::make_shared<Sound>();
  if (!boss_intro_sound_->Load("boss_intro.mp3", false))
    return false;

  boss_explosion_sound_ = std::make_shared<Sound>();
  if (!boss_explosion_sound_->Load("boss_explosion.mp3", false))
    return false;

  explosion_sound_ = std::make_shared<Sound>();
  if (!explosion_sound_->Load("explosion.mp3", false))
    return false;

  stealth_sound_ = std::make_shared<Sound>();
  if (!stealth_sound_->Load("stealth.mp3", false))
    return false;

  shield_on_sound_ = std::make_shared<Sound>();
  if (!shield_on_sound_->Load("shield.mp3", false))
    return false;

  hit_sound_ = std::make_shared<Sound>();
  if (!hit_sound_->Load("hit.mp3", false))
    return false;

  power_up_spawn_sound_ = std::make_shared<Sound>();
  if (!power_up_spawn_sound_->Load("powerup-spawn.mp3", false))
    return false;

  power_up_pick_sound_ = std::make_shared<Sound>();
  if (!power_up_pick_sound_->Load("powerup-pick.mp3", false))
    return false;

  if (!CreateRenderResources())
    return false;

  boss_.SetZOrder(10);
  boss_animator_.Attach(&boss_);

  boss_intro_.SetSound(boss_intro_sound_);
  boss_intro_.SetVariate(false);
  boss_intro_.SetSimulateStereo(false);

  return true;
}

void Enemy::Update(float delta_time) {
  if (!progress_paused_) {
    if (boss_fight_)
      UpdateBoss(delta_time);
    else
      UpdateWave(delta_time);
  }

  Random& rnd = Engine::Get().GetRandomGenerator();

  chromatic_aberration_offset_ += 0.8f * delta_time;

  // Update enemy units.
  for (auto it = enemies_.begin(); it != enemies_.end();) {
    if (it->marked_for_removal) {
      it = enemies_.erase(it);
      continue;
    }

    if (it->chromatic_aberration_active_) {
      it->sprite.SetCustomUniform(
          "aberration_offset", Lerp(0.0f, 0.01f, chromatic_aberration_offset_));
    }
#if defined(LOAD_TEST)
    else if (it->kill_timer <= 0 &&
             it->movement_animator.GetTime(Animator::kMovement) > 0.7f) {
      TakeDamage(&*it, 1);
    }
#endif

    if (it->kill_timer > 0) {
      it->kill_timer -= delta_time;
      if (it->kill_timer <= 0)
        TakeDamage(&*it, 100);
    } else if ((it->enemy_type == kEnemyType_LightSkull ||
                it->enemy_type == kEnemyType_DarkSkull ||
                it->enemy_type == kEnemyType_Tank) &&
               it->sprite_animator.IsPlaying(Animator::kFrames) &&
               !it->idle2_anim && rnd.Roll(200) == 1) {
      // Play random idle animation.
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
    it++;
  }

#if defined(LOAD_TEST)
  if (boss_fight_ && IsBossAlive() && boss_spawn_time_ > 40)
    KillBoss();
#endif
}

void Enemy::Pause(bool pause) {
  for (auto& e : enemies_) {
    e.sprite_animator.PauseOrResumeAll(pause);
    e.target_animator.PauseOrResumeAll(pause);
    e.blast_animator.PauseOrResumeAll(pause);
    e.shield_animator.PauseOrResumeAll(pause);
    e.health_animator.PauseOrResumeAll(pause);
    e.score_animator.PauseOrResumeAll(pause);
    e.movement_animator.PauseOrResumeAll(pause);
  }
  boss_animator_.PauseOrResumeAll(pause);
}

void Enemy::ContextLost() {
  CreateShaders();
}

bool Enemy::HasTarget(DamageType damage_type) {
  DCHECK(damage_type > kDamageType_Invalid && damage_type < kDamageType_Any);

  return GetTarget(damage_type) ? true : false;
}

Vector2f Enemy::GetTargetPos(DamageType damage_type) {
  DCHECK(damage_type > kDamageType_Invalid && damage_type < kDamageType_Any);

  EnemyUnit* target = GetTarget(damage_type);
  if (target)
    return target->sprite.GetPosition() -
           Vector2f(0, target->sprite.GetSize().y / 2.5f);
  return {0, 0};
}

void Enemy::SelectTarget(DamageType damage_type,
                         const Vector2f& origin,
                         const Vector2f& dir) {
  DCHECK(damage_type > kDamageType_Invalid && damage_type < kDamageType_Any);

  if (progress_paused_)
    return;

  std::vector<std::tuple<EnemyUnit*, float, float, Vector2f>> candidates;

  for (auto& e : enemies_) {
    if (e.hit_points <= 0 || e.marked_for_removal || e.stealth_active)
      continue;

    if (e.targetted_by_weapon_[damage_type]) {
      e.targetted_by_weapon_[damage_type] = false;
      e.target.SetVisible(false);
      e.target_animator.Stop(Animator::kAllAnimations);
    }

    Vector2f weapon_enemy_dir = e.sprite.GetPosition() - origin;
    float weapon_enemy_dist = weapon_enemy_dir.Length();
    weapon_enemy_dir.Normalize();
    float cos_theta = weapon_enemy_dir.DotProduct(dir);

    candidates.push_back(
        std::make_tuple(&e, cos_theta, weapon_enemy_dist, weapon_enemy_dir));
  }

  if (candidates.empty())
    return;

  auto all_candidates = candidates;

  for (auto it = candidates.begin(); it != candidates.end();) {
    auto [cand_enemy, cand_cos_theta, cand_dist, cand_dir] = *it;

    auto oit = candidates.begin();
    for (; oit != candidates.end(); ++oit) {
      auto [other_enemy, other_cos_theta, other_dist, orther_dir] = *oit;

      if (cand_enemy == other_enemy || cand_dist < other_dist)
        continue;

      // Remove obstructed units.
      if (base::Intersection(other_enemy->sprite.GetPosition(),
                             other_enemy->sprite.GetSize(), origin, cand_dir)) {
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

  // Sort by cos-theta.
  std::sort(candidates.begin(), candidates.end(),
            [](auto& a, auto& b) { return std::get<1>(a) > std::get<1>(b); });

  constexpr float threshold = 0.95f;

  EnemyUnit* best_enemy = nullptr;
  for (auto& cand : candidates) {
    auto [cand_enemy, cos_theta, cand_dist, cand_dir] = cand;
    if ((cand_enemy->damage_type == damage_type ||
         cand_enemy->damage_type == kDamageType_Any) &&
        cos_theta > threshold) {
      best_enemy = cand_enemy;
      break;
    }
  }

  if (!best_enemy && std::get<1>(candidates[0]) > threshold)
    best_enemy = std::get<0>(candidates[0]);

  if (!best_enemy) {
    // Sort by distance.
    std::sort(all_candidates.begin(), all_candidates.end(),
              [](auto& a, auto& b) { return std::get<2>(a) > std::get<2>(b); });

    if (base::Intersection(std::get<0>(all_candidates[0])->sprite.GetPosition(),
                           std::get<0>(all_candidates[0])->sprite.GetSize(),
                           origin, dir))
      best_enemy = std::get<0>(all_candidates[0]);
  }

  if (best_enemy) {
    best_enemy->targetted_by_weapon_[damage_type] = true;
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
  DCHECK(damage_type > kDamageType_Invalid && damage_type < kDamageType_Any);

  EnemyUnit* target = GetTarget(damage_type);
  if (target) {
    target->targetted_by_weapon_[damage_type] = false;
    target->target.SetVisible(false);
    target->target_animator.Stop(Animator::kAllAnimations);
  }
}

void Enemy::HitTarget(DamageType damage_type) {
  DCHECK(damage_type > kDamageType_Invalid && damage_type < kDamageType_Any);

  if (progress_paused_)
    return;

  EnemyUnit* target = GetTarget(damage_type);

  if (!target)
    return;

  target->target.SetVisible(false);
  target->target_animator.Stop(Animator::kAllAnimations);
  target->targetted_by_weapon_[damage_type] = false;

  if (target->damage_type != kDamageType_Any &&
      target->damage_type != damage_type) {
    // No shield until wave 4.
    if (wave_ <= 3)
      return;

    if (!target->shield_active) {
      target->shield_active = true;
      target->shield.SetVisible(true);

      // Play intro anim and start shield timer.
      target->shield_animator.Stop(Animator::kAllAnimations);
      target->shield.SetFrame(0);
      target->shield_animator.SetFrames(4, 12);
      target->shield_animator.SetTimer(1.0f);
      target->shield_animator.Play(Animator::kFrames | Animator::kTimer, false);
      target->shield_animator.SetEndCallback(
          Animator::kFrames, [&, target]() -> void {
            // Play loop anim.
            target->shield.SetFrame(4);
            target->shield_animator.SetFrames(4, 12);
            target->shield_animator.Play(Animator::kFrames, true);
            target->shield_animator.SetEndCallback(Animator::kFrames, nullptr);
          });
      target->shield_animator.SetEndCallback(
          Animator::kTimer, [&, target]() -> void {
            // Timeout. Remove shield if sill active.
            if (target->shield_active) {
              target->shield_active = false;
              target->shield_animator.Stop(Animator::kFrames);
              target->shield_animator.Play(Animator::kBlending, false);
            }
          });

      target->shield_on.Play(false);
    } else {
      // Restart shield timer.
      target->shield_animator.Stop(Animator::kTimer);
      target->shield_animator.SetTimer(0.6f);
      target->shield_animator.Play(Animator::kTimer, false);
    }
    return;
  }

  TakeDamage(target, 1);
}

bool Enemy::IsBossAlive() const {
  return boss_fight_ && boss_.GetFrame() < 9;
}

void Enemy::PauseProgress() {
  progress_paused_ = true;
}

void Enemy::ResumeProgress() {
  progress_paused_ = false;
}

void Enemy::StopAllEnemyUnits(bool chromatic_aberration_effect) {
  for (auto& e : enemies_) {
    if (e.enemy_type > kEnemyType_Unit_Last || e.marked_for_removal ||
        e.hit_points == 0)
      continue;

    if (chromatic_aberration_effect) {
      e.sprite.SetCustomShader(chromatic_aberration_.get());
      e.chromatic_aberration_active_ = true;
    }

    e.movement_animator.Pause(Animator::kMovement);
    e.freeze_ = true;

    if (e.stealth_active) {
      e.sprite_animator.Stop(Animator::kAllAnimations | Animator::kTimer);
      e.sprite_animator.SetBlending({1, 1, 1, 1}, 0.5f);
      e.sprite_animator.SetEndCallback(Animator::kBlending, nullptr);
      e.sprite_animator.Play(Animator::kBlending, false);
      e.sprite_animator.Play(Animator::kFrames, true);
    }
  }

  if (chromatic_aberration_effect)
    chromatic_aberration_offset_ = 0.0f;
}

void Enemy::KillAllEnemyUnits(bool randomize_order) {
  Engine& engine = Engine::Get();
  Demo* game = static_cast<Demo*>(engine.GetGame());

  for (auto& e : enemies_) {
    if (!e.marked_for_removal && e.hit_points > 0 &&
        e.enemy_type <= kEnemyType_Unit_Last) {
      if (randomize_order) {
        e.kill_timer = Lerp(0.0f, engine.GetScreenSize().y * 0.5f * 0.15f,
                            engine.GetRandomGenerator().GetFloat());
      } else {
        float dist = e.sprite.GetPosition().y -
                     game->GetPlayer().GetWeaponPos(kDamageType_Green).y;
        e.kill_timer = dist * 0.15f;
      }
    }
  }
}

void Enemy::RemoveAll() {
  for (auto& e : enemies_) {
    if (e.enemy_type == kEnemyType_Boss) {
      e.marked_for_removal = true;
    } else if (!e.marked_for_removal && e.hit_points > 0) {
      e.sprite_animator.SetEndCallback(
          Animator::kBlending, [&]() -> void { e.marked_for_removal = true; });
      e.sprite_animator.SetBlending({1, 1, 1, 0}, 0.3f);
      e.sprite_animator.Play(Animator::kBlending, false);
    }
  }

  // Hide boss if not already hiding.
  if (boss_.IsVisible() &&
      !boss_animator_.IsPlaying(Animator::kTimer | Animator::kMovement)) {
    boss_animator_.SetEndCallback(Animator::kMovement, [&]() -> void {
      boss_animator_.SetVisible(false);
    });
    boss_animator_.SetMovement({0, boss_.GetSize().y * 0.99f}, 1);
    boss_animator_.Play(Animator::kMovement, false);
  }
}

void Enemy::KillBoss() {
  for (auto& e : enemies_) {
    if (e.enemy_type == kEnemyType_Boss) {
      TakeDamage(&e, 1000);
      break;
    }
  }
}

void Enemy::Reset() {
  seconds_since_last_power_up_ = 0;
  seconds_to_next_power_up_ = 0;
}

void Enemy::OnWaveStarted(int wave, bool boss_fight) {
  num_enemies_killed_in_current_wave_ = 0;
  seconds_since_last_spawn_ = {0, 0, 0, 0};
  seconds_to_next_spawn_ = {0, 0, 0, 0};
  spawn_factor_ = 0.3077f - (0.0538f * log((float)wave));
  last_spawn_col_ = 0;
  progress_paused_ = false;
  wave_ = wave;
  boss_fight_ = boss_fight;

  if (boss_fight) {
    boss_spawn_time_ = 0;
    boss_spawn_time_factor_ = [wave]() -> float {
      if (wave <= 6)
        return 0.4f;
      if (wave <= 9)
        return 0.6f;
      if (wave <= 12)
        return 1.0f;
      return 1.6f;
    }();
    DLOG << "boss_spawn_time_factor_: " << boss_spawn_time_factor_;
    SpawnBoss();
  }
}

bool Enemy::CheckSpawnPos(Vector2f pos, SpeedType speed_type) {
  for (auto& e : enemies_) {
    if (e.hit_points <= 0 || e.marked_for_removal || e.stealth_active ||
        e.enemy_type > kEnemyType_Unit_Last || e.speed_type != speed_type)
      continue;

    // Check for collision.
    float sy = e.sprite.GetSize().y;
    Vector2f spawn_pos = pos + Vector2f(0, sy);

    bool gc = (spawn_pos - e.sprite.GetPosition()).Length() < sy * 0.8f;
    bool tc = e.movement_animator.GetTime(Animator::kMovement) <= 0.06f;

    if (gc && tc)
      return false;
  }

  return true;
}

bool Enemy::CheckTeleportPos(EnemyUnit* enemy) {
  Vector2f pos = enemy->sprite.GetPosition();
  float t = enemy->movement_animator.GetTime(Animator::kMovement);

  for (auto& e : enemies_) {
    if (&e == enemy || e.hit_points <= 0 || e.marked_for_removal ||
        e.stealth_active || e.enemy_type > kEnemyType_Unit_Last ||
        e.speed_type != enemy->speed_type)
      continue;

    if (e.enemy_type == kEnemyType_Bug &&
        !e.movement_animator.IsPlaying(Animator::kMovement))
      continue;

    bool gc =
        (pos - e.sprite.GetPosition()).Length() < e.sprite.GetSize().y * 0.8f;
    bool tc =
        fabs(t - e.movement_animator.GetTime(Animator::kMovement)) <= 0.04f;

    if (gc && tc)
      return false;
  }

  return true;
}

void Enemy::SpawnUnit(EnemyType enemy_type,
                      DamageType damage_type,
                      const Vector2f& pos,
                      float speed,
                      SpeedType speed_type) {
  DCHECK(
      (enemy_type > kEnemyType_Invalid && enemy_type <= kEnemyType_Unit_Last) ||
      enemy_type == kEnemyType_PowerUp);
  DCHECK(damage_type > kDamageType_Invalid && damage_type < kDamageType_Max);

  Engine& engine = Engine::Get();
  Demo* game = static_cast<Demo*>(engine.GetGame());

  auto& e = enemies_.emplace_back();
  e.enemy_type = enemy_type;
  e.damage_type = damage_type;
  e.speed_type = speed_type;

  switch (enemy_type) {
    case kEnemyType_LightSkull:
      e.total_health = e.hit_points = 1;
      e.sprite.Create("skull_tex", {10, 13}, 100, 100);
      break;
    case kEnemyType_DarkSkull:
      e.total_health = e.hit_points = 2;
      e.sprite.Create("skull_tex", {10, 13}, 100, 100);
      break;
    case kEnemyType_Tank:
      e.total_health = e.hit_points = 5;
      e.sprite.Create("skull_tex", {10, 13}, 100, 100);
      break;
    case kEnemyType_Bug:
      e.total_health = e.hit_points = 3;
      e.sprite.Create("bug_tex", {10, 4});
      break;
    case kEnemyType_PowerUp:
      e.total_health = e.hit_points = 1;
      e.sprite.Create("crate_tex", {8, 3});
      break;
    default:
      NOTREACHED << "- Unkown enemy type: " << enemy_type;
  }

  e.sprite.SetZOrder(11);
  e.sprite.SetVisible(true);
  Vector2f spawn_pos = pos + Vector2f(0, e.sprite.GetSize().y / 2);
  e.sprite.SetPosition(spawn_pos);

  e.sprite.SetFrame(idle1_frame_start[enemy_type][damage_type]);
  e.sprite_animator.SetFrames(idle1_frame_count[enemy_type][damage_type],
                              idle_frame_speed);

  e.sprite.SetColor({1, 1, 1, 0});
  e.sprite_animator.SetBlending({1, 1, 1, 1}, 0.3f);

  e.sprite_animator.Attach(&e.sprite);
  e.sprite_animator.Play(Animator::kBlending, false);
  e.sprite_animator.Play(Animator::kFrames, true);

  e.target.Create("target_tex", {6, 2});
  e.target.SetZOrder(12);
  e.target.SetPosition(spawn_pos);

  if (enemy_type == kEnemyType_PowerUp)
    e.blast.Create("crate_tex", {8, 3});
  else
    e.blast.Create("blast_tex", {6, 2});
  e.blast.SetZOrder(12);
  e.blast.SetPosition(spawn_pos);

  e.shield.Create("shield_tex", {4, 2});
  e.shield.SetZOrder(11);
  e.shield.SetPosition(spawn_pos);

  e.health_base.SetZOrder(11);
  e.health_base.SetSize(e.sprite.GetSize() * Vector2f(0.6f, 0.01f));
  e.health_base.SetPosition(spawn_pos);
  e.health_base.PlaceToBottomOf(e.sprite);
  e.health_base.SetColor({0.5f, 0.5f, 0.5f, 1});

  e.health_bar.SetZOrder(11);
  e.health_bar.SetSize(e.sprite.GetSize() * Vector2f(0.6f, 0.01f));
  e.health_bar.SetPosition(spawn_pos);
  e.health_bar.PlaceToBottomOf(e.sprite);
  e.health_bar.SetColor({0.161f, 0.89f, 0.322f, 1});

  e.score.Create("score_tex"s + std::to_string(e.enemy_type));
  e.score.SetZOrder(12);
  e.score.Scale(1.1f);
  e.score.SetColor({1, 1, 1, 1});
  e.score.SetPosition(spawn_pos);

  e.target_animator.Attach(&e.target);

  e.blast_animator.SetEndCallback(Animator::kFrames, [&]() -> void {
    e.blast.SetVisible(false);
    if (e.enemy_type == kEnemyType_PowerUp)
      e.marked_for_removal = true;
  });
  if (enemy_type == kEnemyType_PowerUp) {
    e.blast.SetFrame(8);
    e.blast_animator.SetFrames(13, 18);
  } else if (damage_type == kDamageType_Green) {
    e.blast.SetFrame(0);
    e.blast_animator.SetFrames(6, 18);
  } else {
    e.blast.SetFrame(6);
    e.blast_animator.SetFrames(6, 18);
  }
  e.blast_animator.Attach(&e.blast);

  e.shield_animator.Attach(&e.shield);
  e.shield_animator.SetBlending({1, 1, 1, 0}, 0.15f, nullptr);
  e.shield_animator.SetEndCallback(Animator::kBlending, [&]() -> void {
    e.shield_animator.Stop(Animator::kAllAnimations | Animator::kTimer);
    e.shield.SetVisible(false);
  });

  SetupFadeOutAnim(e.health_animator, 1);
  e.health_animator.Attach(&e.health_base);
  e.health_animator.Attach(&e.health_bar);

  SetupFadeOutAnim(e.score_animator, 0.5f);
  e.score_animator.SetMovement({0, engine.GetScreenSize().y / 2}, 2.0f);
  e.score_animator.SetEndCallback(
      Animator::kMovement, [&]() -> void { e.marked_for_removal = true; });
  e.score_animator.Attach(&e.score);

  float max_distance =
      spawn_pos.y - game->GetPlayer().GetWeaponPos(kDamageType_Green).y;
  if (enemy_type == kEnemyType_PowerUp)
    max_distance /= 2;

  Animator::Interpolator interpolator;
  if (boss_fight_)
    interpolator = std::bind(CatmullRom, std::placeholders::_1, 2.5f, 1.5f);
  else if (enemy_type == kEnemyType_PowerUp)
    interpolator = std::bind(CatmullRom, std::placeholders::_1, -9.0, 1.35f);
  else
    interpolator = std::bind(Acceleration, std::placeholders::_1, -0.15f);
  e.movement_animator.SetMovement({0, -max_distance}, speed, interpolator);
  e.movement_animator.SetEndCallback(Animator::kMovement, [&]() -> void {
    // Enemy has reached the player.
    e.hit_points = 0;
    e.target.SetVisible(false);
    e.blast.SetVisible(false);
    if (e.enemy_type == kEnemyType_PowerUp)
      seconds_to_next_power_up_ *= 0.5f;
    else
      static_cast<Demo*>(engine.GetGame())->GetPlayer().TakeDamage(1);
    e.sprite_animator.SetEndCallback(
        Animator::kBlending, [&]() -> void { e.marked_for_removal = true; });
    e.sprite_animator.SetBlending({1, 1, 1, 0}, 0.3f);
    e.sprite_animator.Play(Animator::kBlending, false);
  });
  e.movement_animator.Attach(&e.sprite);
  e.movement_animator.Attach(&e.target);
  e.movement_animator.Attach(&e.blast);
  e.movement_animator.Attach(&e.shield);
  e.movement_animator.Attach(&e.health_base);
  e.movement_animator.Attach(&e.health_bar);
  e.movement_animator.Attach(&e.score);
  e.movement_animator.Play(Animator::kMovement, false);

  if (e.enemy_type == kEnemyType_PowerUp) {
    e.explosion.SetSound(power_up_pick_sound_);

    e.spawn.SetSound(power_up_spawn_sound_);
    e.spawn.SetMaxAplitude(2.0f);
    e.spawn.Play(false);
  } else {
    e.explosion.SetSound(explosion_sound_);
    e.explosion.SetVariate(true);
    e.explosion.SetSimulateStereo(true);
    e.explosion.SetMaxAplitude(0.9f);
  }

  e.stealth.SetSound(stealth_sound_);
  e.stealth.SetVariate(false);
  e.stealth.SetSimulateStereo(false);
  e.stealth.SetMaxAplitude(0.7f);

  e.shield_on.SetSound(shield_on_sound_);
  e.shield_on.SetVariate(false);
  e.shield_on.SetSimulateStereo(false);
  e.shield_on.SetMaxAplitude(0.5f);

  e.hit.SetSound(hit_sound_);
  e.hit.SetVariate(true);
  e.hit.SetSimulateStereo(false);
  e.hit.SetMaxAplitude(0.5f);
}

void Enemy::SpawnBoss() {
  // Setup visual sprite of the boss.
  int boss_id = (wave_ / 3) % 3;
  if (boss_id == 0)
    boss_id = 3;
  boss_.Create("boss_tex"s + std::to_string(boss_id), {4, 3});
  boss_.SetVisible(true);
  boss_.SetPosition(Engine::Get().GetScreenSize() * Vector2f(0, 0.5f) +
                    boss_.GetSize() * Vector2f(0, 2.0f));
  boss_animator_.SetMovement(
      {0, boss_.GetSize().y * -2.4f}, 5,
      std::bind(Acceleration, std::placeholders::_1, -1));
  boss_.SetFrame(0);
  boss_animator_.SetFrames(8, 16);

  boss_animator_.SetEndCallback(Animator::kMovement, [&]() -> void {
    Engine& engine = Engine::Get();
    Demo* game = static_cast<Demo*>(engine.GetGame());

    // Spwawn a stationary enemy unit for the boss.
    auto& e = enemies_.emplace_front();
    e.enemy_type = kEnemyType_Boss;
    e.damage_type = kDamageType_Any;
    e.total_health = e.hit_points =
        -15.0845f + 41.1283f * log((float)game->wave());
    DLOG << " Boss health: " << e.total_health;

    Vector2f hit_box_pos =
        boss_.GetPosition() - boss_.GetSize() * Vector2f(0, 0.2f);

    // Just a hit box, no visual sprite.
    e.sprite.SetPosition(hit_box_pos);
    e.sprite.SetSize(boss_.GetSize() * 0.3f);

    e.target.Create("target_tex", {6, 2});
    e.target.SetZOrder(12);
    e.target.SetPosition(hit_box_pos);

    Vector2f health_bar_offset = boss_.GetSize() * Vector2f(0, 0.2f);

    // A thicker and always visible health bar.
    e.health_base.SetZOrder(10);
    e.health_base.SetSize(e.sprite.GetSize() * Vector2f(0.7f, 0.08f));
    e.health_base.SetPosition(hit_box_pos + health_bar_offset);
    e.health_base.PlaceToBottomOf(boss_);
    e.health_base.SetColor({0.5f, 0.5f, 0.5f, 1});
    e.health_base.SetVisible(true);

    e.health_bar.SetZOrder(10);
    e.health_bar.SetSize(e.sprite.GetSize() * Vector2f(0.7f, 0.08f));
    e.health_bar.SetPosition(hit_box_pos + health_bar_offset);
    e.health_bar.PlaceToBottomOf(boss_);
    e.health_bar.SetColor({0.161f, 0.89f, 0.322f, 1});
    e.health_bar.SetVisible(true);

    e.score.Create("score_tex"s + std::to_string(e.enemy_type));
    e.score.SetZOrder(12);
    e.score.SetColor({1, 1, 1, 1});
    e.score.SetPosition(hit_box_pos);

    e.target_animator.Attach(&e.target);

    SetupFadeOutAnim(e.score_animator, 0.5f);
    e.score_animator.SetMovement({0, Engine::Get().GetScreenSize().y / 2},
                                 2.0f);
    e.score_animator.SetEndCallback(
        Animator::kMovement, [&]() -> void { e.marked_for_removal = true; });
    e.score_animator.Attach(&e.score);

    e.explosion.SetSound(boss_explosion_sound_);
    e.explosion.SetVariate(false);
    e.explosion.SetSimulateStereo(false);

    e.hit.SetSound(hit_sound_);
    e.hit.SetVariate(true);
    e.hit.SetSimulateStereo(false);
    e.hit.SetMaxAplitude(0.5f);
  });
  boss_animator_.Play(Animator::kFrames, true);
  boss_animator_.Play(Animator::kMovement, false);

  boss_intro_.Play(false);
}

void Enemy::TakeDamage(EnemyUnit* target, int damage) {
  DCHECK(!target->marked_for_removal);

  if (target->hit_points <= 0)
    return;

  Engine::Get().Vibrate(30);

  if (target->shield_active) {
    // Remove shield.
    target->shield_active = false;
    target->shield_animator.Stop(Animator::kFrames | Animator::kTimer);
    target->shield_animator.Play(Animator::kBlending, false);

    if (--damage == 0)
      return;
  }

  target->hit_points -= damage;

  target->blast.SetVisible(true);
  target->blast_animator.Play(Animator::kFrames, false);

  if (target->hit_points <= 0) {
    Engine& engine = Engine::Get();
    Demo* game = static_cast<Demo*>(engine.GetGame());

    if (target->enemy_type != kEnemyType_PowerUp)
      ++num_enemies_killed_in_current_wave_;

    target->sprite.SetVisible(false);
    target->health_base.SetVisible(false);
    target->health_bar.SetVisible(false);
    target->target.SetVisible(false);

    target->spawn.Stop(0.5f);

    if (target->enemy_type == kEnemyType_PowerUp) {
      // Move power-up sprite towards player.
      float distance = target->sprite.GetPosition().y -
                       game->GetPlayer().GetWeaponPos(kDamageType_Green).y;
      target->movement_animator.SetMovement(
          {0, -distance}, 0.7f,
          std::bind(Acceleration, std::placeholders::_1, 1));
      target->movement_animator.Stop(Animator::kMovement);
      target->movement_animator.SetEndCallback(Animator::kMovement, nullptr);
      target->movement_animator.Play(Animator::kMovement, false);
    } else {
      // Stop enemy sprite and play score animation.
      target->score.SetVisible(true);
      target->score_animator.Play(Animator::kTimer | Animator::kMovement,
                                  false);
      target->movement_animator.Pause(Animator::kMovement);
    }

    int score = GetScore(target->enemy_type);
    if (score)
      game->AddScore(score);

    target->explosion.Play(false);

    if (target->enemy_type == kEnemyType_PowerUp) {
      if (damage == 1)
        game->GetPlayer().AddNuke(1);
    } else if (target->enemy_type == kEnemyType_Boss) {
      // Play dead animation and move away the boss.
      boss_animator_.Stop(Animator::kFrames | Animator::kTimer);
      boss_animator_.SetEndCallback(Animator::kMovement, [&]() -> void {
        boss_animator_.SetVisible(false);
      });
      boss_animator_.SetMovement({0, boss_.GetSize().y * 0.99f}, 4);
      boss_.SetFrame(9);
      boss_animator_.SetFrames(2, 12);
      boss_animator_.SetEndCallback(Animator::kTimer, [&]() -> void {
        boss_animator_.Stop(Animator::kFrames);
        boss_animator_.Play(Animator::kMovement, false);
        boss_.SetFrame(11);
      });
      boss_animator_.SetTimer(1.25f);
      boss_animator_.Play(Animator::kFrames | Animator::kTimer, true);
    }
  } else {
    Vector2f s = target->health_base.GetSize();
    s.x *= (float)target->hit_points / (float)target->total_health;
    float t = (s.x - target->health_bar.GetSize().x) / 2;
    target->health_bar.SetSize(s);
    target->health_bar.Translate({t, 0});

    target->health_base.SetVisible(true);
    target->health_bar.SetVisible(true);

    target->health_animator.Stop(Animator::kTimer | Animator::kBlending);
    target->health_animator.Play(Animator::kTimer, false);

    if (target->enemy_type == kEnemyType_Bug &&
        target->hit_points == target->total_health - 1) {
      // Stealth and teleport.
      target->stealth_active = true;
      target->movement_animator.Pause(Animator::kMovement);
      target->sprite_animator.Pause(Animator::kFrames);

      Random& rnd = Engine::Get().GetRandomGenerator();
      float stealth_timer = Lerp(2.0f, 5.0f, rnd.GetFloat());
      target->sprite_animator.SetEndCallback(
          Animator::kTimer, [&, target]() -> void {
            // No horizontal teleport in boss fight.
            if (!boss_fight_) {
              float x = SnapSpawnPosX(rnd.Roll(4) - 1);
              TranslateEnemyUnit(*target,
                                 {x - target->sprite.GetPosition().x, 0});
            }

            // Vertical teleport (wave 6+).
            float ct = target->movement_animator.GetTime(Animator::kMovement);
            if (wave_ >= 6 && ct < 0.6f) {
              float t = Lerp(0.0f, 0.5f, rnd.GetFloat());
              float nt = std::min(ct + t, 0.6f);
              target->movement_animator.SetTime(Animator::kMovement, nt, true);
            }

            target->stealth_active = false;
            target->sprite_animator.SetBlending({1, 1, 1, 1}, 1.0f);
            target->sprite_animator.Play(Animator::kBlending, false);
            target->sprite_animator.SetEndCallback(
                Animator::kBlending, [&]() -> void {
                  if (target->freeze_) {
                    target->sprite_animator.Play(Animator::kFrames, true);
                  } else if (CheckTeleportPos(target)) {
                    target->movement_animator.Play(Animator::kMovement, false);
                    target->sprite_animator.Play(Animator::kFrames, true);
                  } else {
                    // Try again soon.
                    target->sprite_animator.SetBlending({1, 1, 1, 1}, 0.001f);
                    target->sprite_animator.Play(Animator::kBlending, false);
                  }
                });
          });

      target->sprite_animator.SetTimer(stealth_timer);
      target->sprite_animator.SetBlending({1, 1, 1, 0}, 1.5f);
      target->sprite_animator.Play(Animator::kBlending | Animator::kTimer,
                                   false);

      target->stealth.Play(false);
    } else {
      target->hit.Play(false);
    }

    if (target->enemy_type == kEnemyType_Boss) {
      // Play damage animation.
      boss_animator_.Stop(Animator::kFrames | Animator::kTimer);
      boss_.SetFrame(8);
      boss_animator_.SetFrames(1, 1);
      boss_animator_.SetEndCallback(Animator::kTimer, [&]() -> void {
        boss_animator_.Stop(Animator::kFrames);
        boss_.SetFrame(0);
        boss_animator_.SetFrames(8, 12);
        boss_animator_.Play(Animator::kFrames, true);
      });
      boss_animator_.SetTimer(0.2f);
      boss_animator_.Play(Animator::kFrames | Animator::kTimer, true);
    }
  }
}

void Enemy::UpdateWave(float delta_time) {
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
          Lerp(kSpawnPeriod[i][0] * spawn_factor_,
               kSpawnPeriod[i][1] * spawn_factor_, rnd.GetFloat());
      break;
    }
  }

  if (enemy_type != kEnemyType_Invalid) {
    // Spawn only light enemies during the first 4 waves. Then gradually
    // introduce harder enemy types.
    if (enemy_type != kEnemyType_LightSkull && wave_ <= 3)
      enemy_type = wave_ == 1 ? kEnemyType_Invalid : kEnemyType_LightSkull;
    else if (enemy_type > kEnemyType_DarkSkull && wave_ == 4)
      enemy_type = kEnemyType_LightSkull;
    else if (enemy_type == kEnemyType_Tank && wave_ <= 6)
      enemy_type = kEnemyType_LightSkull;
  }

  if (enemy_type != kEnemyType_Invalid) {
    DamageType damage_type = enemy_type == kEnemyType_Tank
                                 ? kDamageType_Any
                                 : (DamageType)(rnd.Roll(2) - 1);

    int col = rnd.Roll(4) - 1;
    if (col == last_spawn_col_)
      col = (col + 1) % 4;
    last_spawn_col_ = col;

    Vector2f s = engine.GetScreenSize();
    float x = SnapSpawnPosX(col);
    Vector2f pos = {x, s.y / 2};

    SpeedType speed_type =
        enemy_type == kEnemyType_Tank
            ? kSpeedType_Slow
            : (rnd.Roll(3) == 1 ? kSpeedType_Fast : kSpeedType_Slow);
    float speed = speed_type == kSpeedType_Slow ? 10.0f : 6.0f;

    if (CheckSpawnPos(pos, speed_type))
      SpawnUnit(enemy_type, damage_type, pos, speed, speed_type);
    else
      seconds_to_next_spawn_[enemy_type] = 0.001f;
  }

  seconds_since_last_power_up_ += delta_time;
  if (seconds_since_last_power_up_ >= seconds_to_next_power_up_) {
    if (seconds_to_next_power_up_ > 0 &&
        static_cast<Demo*>(engine.GetGame())->GetPlayer().nuke_count() < 3) {
      Vector2f s = engine.GetScreenSize();
      Vector2f pos = {0, s.y / 2};
      SpawnUnit(kEnemyType_PowerUp, kDamageType_Any, pos, 6);
    }
    seconds_since_last_power_up_ = 0;
    seconds_to_next_power_up_ =
        Lerp(1.3f * 60.0f, 1.8f * 60.0f, rnd.GetFloat());
  }
}

void Enemy::UpdateBoss(float delta_time) {
  if (boss_animator_.IsPlaying(Animator::kMovement) &&
      boss_animator_.GetTime(Animator::kMovement) < 0.5f)
    return;

  for (int i = 0; i < kEnemyType_Unit_Last + 1; ++i)
    seconds_since_last_spawn_[i] += delta_time;

  Random& rnd = Engine::Get().GetRandomGenerator();

  boss_spawn_time_ += delta_time;
  float boss_spawn_factor =
      0.4f - (0.0684f * log(boss_spawn_time_ * boss_spawn_time_factor_));
  if (boss_spawn_factor < 0.1f)
    boss_spawn_factor = 0.1f;

  DLOG << "boss_spawn_time_: " << boss_spawn_time_
       << " boss_spawn_factor: " << boss_spawn_factor;

  EnemyType enemy_type = kEnemyType_Invalid;

  for (int i = 0; i < kEnemyType_Unit_Last + 1; ++i) {
    if (seconds_since_last_spawn_[i] >= seconds_to_next_spawn_[i]) {
      if (seconds_to_next_spawn_[i] > 0)
        enemy_type = (EnemyType)i;

      seconds_since_last_spawn_[i] = 0;
      seconds_to_next_spawn_[i] =
          Lerp(kSpawnPeriod[i][0] * boss_spawn_factor,
               kSpawnPeriod[i][1] * boss_spawn_factor, rnd.GetFloat());
      break;
    } else if (seconds_to_next_spawn_[i] >
               kSpawnPeriod[i][1] * boss_spawn_factor) {
      seconds_to_next_spawn_[i] =
          Lerp(kSpawnPeriod[i][0] * boss_spawn_factor,
               kSpawnPeriod[i][1] * boss_spawn_factor, rnd.GetFloat());
    }
  }

  if (enemy_type != kEnemyType_Invalid && boss_spawn_factor > 0.11f) {
    // Spawn only light enemies during the first boss fight. Then gradually
    // introduce harder enemy types.
    if (enemy_type != kEnemyType_LightSkull && wave_ == 3)
      enemy_type = enemy_type == kEnemyType_DarkSkull ? kEnemyType_LightSkull
                                                      : kEnemyType_Invalid;
    else if (enemy_type == kEnemyType_Tank && wave_ == 6)
      enemy_type = kEnemyType_Invalid;
  }

  if (enemy_type == kEnemyType_Invalid)
    return;

  DamageType damage_type = enemy_type == kEnemyType_Tank
                               ? kDamageType_Any
                               : (DamageType)(rnd.Roll(2) - 1);

  int col = (last_spawn_col_++) % 2;
  float offset = Lerp(boss_.GetSize().x * -0.12f, boss_.GetSize().x * 0.12f,
                      rnd.GetFloat());
  float x = (boss_.GetSize().x / 3) * (col ? 1 : -1) + offset;
  Vector2f pos = {x, boss_.GetPosition().y - boss_.GetSize().y / 2};

  SpeedType speed_type =
      enemy_type == kEnemyType_Tank
          ? kSpeedType_Slow
          : (rnd.Roll(3) == 1 ? kSpeedType_Fast : kSpeedType_Slow);
  float speed = speed_type == kSpeedType_Slow ? 10.0f : 6.0f;

  if (CheckSpawnPos(pos, speed_type))
    SpawnUnit(enemy_type, damage_type, pos, speed, speed_type);
  else
    seconds_to_next_spawn_[enemy_type] = 0.001f;
}

Enemy::EnemyUnit* Enemy::GetTarget(DamageType damage_type) {
  for (auto& e : enemies_) {
    if (e.targetted_by_weapon_[damage_type] && e.hit_points > 0 &&
        !e.marked_for_removal)
      return &e;
  }
  return nullptr;
}

int Enemy::GetScore(EnemyType enemy_type) {
  DCHECK(enemy_type > kEnemyType_Invalid && enemy_type < kEnemyType_Max);
  return enemy_scores[enemy_type];
}

std::unique_ptr<Image> Enemy::GetScoreImage(EnemyType enemy_type) {
  const Font& font = static_cast<Demo*>(Engine::Get().GetGame())->GetFont();

  int score = GetScore(enemy_type);
  if (!score)
    return nullptr;

  std::string text = std::to_string(score);
  int width, height;
  font.CalculateBoundingBox(text.c_str(), width, height);

  auto image = std::make_unique<Image>();
  image->Create(width, height);
  image->Clear({1, 1, 1, 0});

  font.Print(0, 0, text.c_str(), image->GetBuffer(), image->GetWidth());

  image->Compress();
  return image;
}

bool Enemy::CreateRenderResources() {
  if (!CreateShaders())
    return false;

  Engine::Get().SetImageSource("skull_tex", "enemy_anims_01_frames_ok.png",
                               true);
  Engine::Get().SetImageSource("bug_tex", "enemy_anims_02_frames_ok.png", true);
  Engine::Get().SetImageSource("boss_tex1", "Boss_ok.png", true);
  Engine::Get().SetImageSource("boss_tex2", "Boss_ok_lvl2.png", true);
  Engine::Get().SetImageSource("boss_tex3", "Boss_ok_lvl3.png", true);
  Engine::Get().SetImageSource("target_tex", "enemy_target_single_ok.png",
                               true);
  Engine::Get().SetImageSource("blast_tex", "enemy_anims_blast_ok.png", true);
  Engine::Get().SetImageSource("shield_tex", "woom_enemy_shield.png", true);
  Engine::Get().SetImageSource("crate_tex", "nuke_pack_OK.png", true);

  for (int i = 0; i < kEnemyType_Max; ++i)
    Engine::Get().SetImageSource(
        "score_tex"s + std::to_string(i),
        std::bind(&Enemy::GetScoreImage, this, (EnemyType)i), true);

  return true;
}

bool Enemy::CreateShaders() {
  auto source = std::make_unique<ShaderSource>();
  if (!source->Load("chromatic_aberration.glsl"))
    return false;
  chromatic_aberration_->Create(std::move(source),
                                Engine::Get().GetQuad()->vertex_description(),
                                Engine::Get().GetQuad()->primitive(), false);
  return true;
}

void Enemy::TranslateEnemyUnit(EnemyUnit& e, const Vector2f& delta) {
  e.sprite.Translate(delta);
  e.target.Translate(delta);
  e.blast.Translate(delta);
  e.shield.Translate(delta);
  e.health_base.Translate(delta);
  e.health_bar.Translate(delta);
  e.score.Translate(delta);
}
