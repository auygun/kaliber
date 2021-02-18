#include "enemy.h"

#include <functional>
#include <limits>

#include "../base/collusion_test.h"
#include "../base/interpolation.h"
#include "../base/log.h"
#include "../engine/engine.h"
#include "../engine/font.h"
#include "../engine/image.h"
#include "../engine/sound.h"
#include "demo.h"

using namespace std::string_literals;

using namespace base;
using namespace eng;

namespace {

constexpr int enemy_frame_start[][3] = {{0, 50, -1},
                                        {13, 33, -1},
                                        {-1, -1, 100}};
constexpr int enemy_frame_count[][3] = {{7, 7, -1}, {6, 6, -1}, {-1, -1, 7}};
constexpr int enemy_frame_speed = 12;

constexpr int enemy_scores[] = {100, 150, 300};

constexpr float kSpawnPeriod[kEnemyType_Max][2] = {{2, 5},
                                                   {15, 25},
                                                   {110, 130}};

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

}  // namespace

Enemy::Enemy() = default;

Enemy::~Enemy() = default;

bool Enemy::Initialize() {
  explosion_sound_ = std::make_shared<Sound>();
  if (!explosion_sound_->Load("explosion.mp3", false))
    return false;

  return CreateRenderResources();
}

void Enemy::Update(float delta_time) {
  if (!waiting_for_next_wave_ && !paused_) {
    if (spawn_factor_interpolator_ < 1) {
      spawn_factor_interpolator_ += delta_time * 0.1f;
      if (spawn_factor_interpolator_ > 1)
        spawn_factor_interpolator_ = 1;
    }

    for (int i = 0; i < kEnemyType_Max; ++i)
      seconds_since_last_spawn_[i] += delta_time;

    SpawnNextEnemy();
  }

  for (auto it = enemies_.begin(); it != enemies_.end();) {
    if (it->marked_for_removal)
      it = enemies_.erase(it);
    else
      ++it;
  }
}

void Enemy::Pause(bool pause) {
  paused_ = pause;
  for (auto& e : enemies_) {
    e.movement_animator.PauseOrResumeAll(pause);
    e.sprite_animator.PauseOrResumeAll(pause);
    e.target_animator.PauseOrResumeAll(pause);
    e.blast_animator.PauseOrResumeAll(pause);
    e.health_animator.PauseOrResumeAll(pause);
    e.score_animator.PauseOrResumeAll(pause);
  }
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
                         const Vector2f& dir,
                         float snap_factor) {
  DCHECK(damage_type > kDamageType_Invalid && damage_type < kDamageType_Any);

  if (waiting_for_next_wave_)
    return;

  EnemyUnit* best_enemy = nullptr;

  float closest_dist = std::numeric_limits<float>::max();
  for (auto& e : enemies_) {
    if (e.hit_points <= 0 || e.marked_for_removal)
      continue;

    if (e.targetted_by_weapon_ == damage_type) {
      e.targetted_by_weapon_ = kDamageType_Invalid;
      e.target.SetVisible(false);
      e.target_animator.Stop(Animator::kAllAnimations);
    }

    if (!base::Intersection(e.sprite.GetPosition(),
                            e.sprite.GetSize() * snap_factor, origin, dir))
      continue;

    Vector2f weapon_enemy_dir = e.sprite.GetPosition() - origin;
    float enemy_weapon_dist = weapon_enemy_dir.Length();
    if (closest_dist > enemy_weapon_dist) {
      closest_dist = enemy_weapon_dist;
      best_enemy = &e;
    }
  }

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
  DCHECK(damage_type > kDamageType_Invalid && damage_type < kDamageType_Any);

  EnemyUnit* target = GetTarget(damage_type);
  if (target) {
    target->targetted_by_weapon_ = kDamageType_Invalid;
    target->target.SetVisible(false);
    target->target_animator.Stop(Animator::kAllAnimations);
  }
}

void Enemy::HitTarget(DamageType damage_type) {
  DCHECK(damage_type > kDamageType_Invalid && damage_type < kDamageType_Any);

  if (waiting_for_next_wave_)
    return;

  EnemyUnit* target = GetTarget(damage_type);

  if (target) {
    target->target.SetVisible(false);
    target->target_animator.Stop(Animator::kAllAnimations);
  }

  if (!target || (target->damage_type != kDamageType_Any &&
                  target->damage_type != damage_type))
    return;

  TakeDamage(target, 1);
}

void Enemy::OnWaveFinished() {
  for (auto& e : enemies_) {
    if (!e.marked_for_removal && e.hit_points > 0)
      e.movement_animator.Pause(Animator::kMovement);
  }
  waiting_for_next_wave_ = true;
}

void Enemy::OnWaveStarted(int wave) {
  for (auto& e : enemies_) {
    if (!e.marked_for_removal && e.hit_points > 0) {
      if (wave == 1)
        e.marked_for_removal = true;
      else
        TakeDamage(&e, 100);
    }
  }
  num_enemies_killed_in_current_wave_ = 0;
  seconds_since_last_spawn_ = {0, 0, 0};
  seconds_to_next_spawn_ = {0, 0, 0};
  spawn_factor_ = 1 / (log10(0.25f * (wave + 4) + 1.468f) * 6);
  spawn_factor_interpolator_ = 0;
  waiting_for_next_wave_ = false;
}

void Enemy::TakeDamage(EnemyUnit* target, int damage) {
  DCHECK(!target->marked_for_removal);
  DCHECK(target->hit_points > 0);

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

    target->explosion_.Play(false);

    Engine& engine = Engine::Get();
    Demo* game = static_cast<Demo*>(engine.GetGame());
    game->AddScore(GetScore(target->enemy_type));
  } else {
    target->targetted_by_weapon_ = kDamageType_Invalid;

    Vector2f s = target->sprite.GetSize() * Vector2f(0.6f, 0.01f);
    s.x *= (float)target->hit_points / (float)target->total_health;
    float t = (s.x - target->health_bar.GetSize().x) / 2;
    target->health_bar.SetSize(s);
    target->health_bar.Translate({t, 0});

    target->health_base.SetVisible(true);
    target->health_bar.SetVisible(true);

    target->health_animator.Stop(Animator::kTimer | Animator::kBlending);
    target->health_animator.Play(Animator::kTimer, false);
  }
}

void Enemy::SpawnNextEnemy() {
  Engine& engine = Engine::Get();
  Random& rnd = engine.GetRandomGenerator();

  float factor = Lerp(1.0f, spawn_factor_, spawn_factor_interpolator_);
  EnemyType enemy_type = kEnemyType_Invalid;

  for (int i = 0; i < kEnemyType_Max; ++i) {
    if (seconds_since_last_spawn_[i] >= seconds_to_next_spawn_[i]) {
      if (seconds_to_next_spawn_[i] > 0)
        enemy_type = (EnemyType)i;

      seconds_since_last_spawn_[i] = 0;
      seconds_to_next_spawn_[i] =
          Lerp(kSpawnPeriod[i][0] * factor, kSpawnPeriod[i][1] * factor,
               rnd.GetFloat());
      break;
    }
  }

  if (enemy_type == kEnemyType_Invalid)
    return;

  DamageType damage_type = enemy_type == kEnemyType_Tank
                               ? kDamageType_Any
                               : (DamageType)(rnd.Roll(2) - 1);

  Vector2f s = engine.GetScreenSize();
  int col;
  col = rnd.Roll(4) - 1;
  if (col == last_spawn_col_)
    col = (col + 1) % 4;
  last_spawn_col_ = col;
  float x = (s.x / 4) / 2 + (s.x / 4) * col - s.x / 2;
  Vector2f pos = {x, s.y / 2};
  float speed =
      enemy_type == kEnemyType_Tank ? 36.0f : (rnd.Roll(4) == 4 ? 6.0f : 10.0f);

  Spawn(enemy_type, damage_type, pos, speed);
}

void Enemy::Spawn(EnemyType enemy_type,
                  DamageType damage_type,
                  const Vector2f& pos,
                  float speed) {
  DCHECK(enemy_type > kEnemyType_Invalid && enemy_type < kEnemyType_Max);
  DCHECK(damage_type > kDamageType_Invalid && damage_type < kDamageType_Max);

  Engine& engine = Engine::Get();
  Demo* game = static_cast<Demo*>(engine.GetGame());

  auto& e = enemies_.emplace_back();
  e.enemy_type = enemy_type;
  e.damage_type = damage_type;
  if (enemy_type == kEnemyType_Skull) {
    e.total_health = e.hit_points = 1;
    e.sprite.Create("skull_tex", {10, 13}, 100, 100);
  } else if (enemy_type == kEnemyType_Bug) {
    e.total_health = e.hit_points = 2;
    e.sprite.Create("bug_tex", {10, 4});
  } else {  // kEnemyType_Tank
    e.total_health = e.hit_points = 6;
    e.sprite.Create("skull_tex", {10, 13}, 100, 100);
  }
  e.sprite.SetZOrder(11);
  e.sprite.SetVisible(true);
  Vector2f spawn_pos = pos + Vector2f(0, e.sprite.GetSize().y / 2);
  e.sprite.SetPosition(spawn_pos);

  e.sprite.SetFrame(enemy_frame_start[enemy_type][damage_type]);
  e.sprite_animator.SetFrames(enemy_frame_count[enemy_type][damage_type],
                              enemy_frame_speed);

  e.sprite_animator.Attach(&e.sprite);
  e.sprite_animator.Play(Animator::kFrames, true);

  e.target.Create("target_tex", {6, 2});
  e.target.SetZOrder(12);
  e.target.SetPosition(spawn_pos);

  e.blast.Create("blast_tex", {6, 2});
  e.blast.SetZOrder(12);
  e.blast.SetPosition(spawn_pos);

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
  e.score.SetColor({1, 1, 1, 1});
  e.score.SetPosition(spawn_pos);

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

  e.movement_animator.SetMovement(
      {0, -max_distance}, speed,
      std::bind(Acceleration, std::placeholders::_1, -0.15f));
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

  e.explosion_.SetSound(explosion_sound_);
  e.explosion_.SetVariate(true);
  e.explosion_.SetSimulateStereo(true);
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
  DCHECK(enemy_type > kEnemyType_Invalid && enemy_type < kEnemyType_Max);
  return enemy_scores[enemy_type];
}

std::unique_ptr<Image> Enemy::GetScoreImage(EnemyType enemy_type) {
  const Font& font = static_cast<Demo*>(Engine::Get().GetGame())->GetFont();

  int score = GetScore(enemy_type);
  std::string text = std::to_string(score);
  int width, height;
  font.CalculateBoundingBox(text.c_str(), width, height);

  auto image = std::make_unique<Image>();
  image->Create(width, height);
  image->Clear({1, 1, 1, 0});

  font.Print(0, 0, text.c_str(), image->GetBuffer(), image->GetWidth());

  return image;
}

bool Enemy::CreateRenderResources() {
  Engine::Get().SetImageSource("skull_tex", "enemy_anims_01_frames_ok.png",
                               true);
  Engine::Get().SetImageSource("bug_tex", "enemy_anims_02_frames_ok.png", true);
  Engine::Get().SetImageSource("target_tex", "enemy_target_single_ok.png",
                               true);
  Engine::Get().SetImageSource("blast_tex", "enemy_anims_blast_ok.png", true);

  for (int i = 0; i < kEnemyType_Max; ++i)
    Engine::Get().SetImageSource(
        "score_tex"s + std::to_string(i),
        std::bind(&Enemy::GetScoreImage, this, (EnemyType)i), true);

  return true;
}
