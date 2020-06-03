#include "enemy.h"

#include <cassert>
#include <functional>
#include <limits>

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

Enemy::Enemy()
    : skull_tex_(Engine::Get().CreateRenderResource<Texture>()),
      bug_tex_(Engine::Get().CreateRenderResource<Texture>()),
      target_tex_(Engine::Get().CreateRenderResource<Texture>()),
      blast_tex_(Engine::Get().CreateRenderResource<Texture>()),
      score_tex_{Engine::Get().CreateRenderResource<Texture>(),
                 Engine::Get().CreateRenderResource<Texture>(),
                 Engine::Get().CreateRenderResource<Texture>()} {}

Enemy::~Enemy() = default;

bool Enemy::Initialize() {
  font_ = Engine::Get().GetAsset<Font>("PixelCaps!.ttf");
  if (!font_)
    return false;

  return CreateRenderResources();
}

void Enemy::ContextLost() {
  CreateRenderResources();
}

void Enemy::Update(float delta_time) {
  if (!waiting_for_next_wave_) {
    if (spawn_factor_interpolator_ < 1) {
      spawn_factor_interpolator_ += delta_time * 0.1f;
      if (spawn_factor_interpolator_ > 1)
        spawn_factor_interpolator_ = 1;
    }

    for (int i = 0; i < kEnemyType_Max; ++i)
      seconds_since_last_spawn_[i] += delta_time;

    SpawnNextEnemy();
  }

  for (auto it = enemies_.begin(); it != enemies_.end(); ++it) {
    if (it->marked_for_removal) {
      it = enemies_.erase(it);
      continue;
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
                         const Vector2& dir,
                         float snap_factor) {
  assert(damage_type > kDamageType_Invalid && damage_type < kDamageType_Any);

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

    if (!base::Intersection(e.sprite.GetOffset(),
                            e.sprite.GetScale() * snap_factor,
                            origin, dir))
      continue;

    Vector2 weapon_enemy_dir = e.sprite.GetOffset() - origin;
    float enemy_weapon_dist = weapon_enemy_dir.Magnitude();
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
  } else {
    target->targetted_by_weapon_ = kDamageType_Invalid;

    Vector2 s = target->sprite.GetScale() * Vector2(0.6f, 0.01f);
    s.x *= (float)target->hit_points / (float)target->total_health;
    float t = (s.x - target->health_bar.GetScale().x) / 2;
    target->health_bar.SetScale(s);
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
                               : (DamageType)(rnd.Roll(0, 1));

  Vector2 s = engine.GetScreenSize();
  int col;
  col = rnd.Roll(0, 3);
  if (col == last_spawn_col_)
    col = (col + 1) % 4;
  last_spawn_col_ = col;
  float x = (s.x / 4) / 2 + (s.x / 4) * col - s.x / 2;
  Vector2 pos = {x, s.y / 2};
  float speed = enemy_type == kEnemyType_Tank
                    ? 36.0f
                    : (rnd.Roll(1, 4) == 4 ? 6.0f : 10.0f);

  Spawn(enemy_type, damage_type, pos, speed);
}

void Enemy::Spawn(EnemyType enemy_type,
                  DamageType damage_type,
                  const Vector2& pos,
                  float speed) {
  assert(enemy_type > kEnemyType_Invalid && enemy_type < kEnemyType_Max);
  assert(damage_type > kDamageType_Invalid && damage_type < kDamageType_Max);

  Engine& engine = Engine::Get();
  Demo* game = static_cast<Demo*>(engine.GetGame());

  auto& e = enemies_.emplace_back();
  e.enemy_type = enemy_type;
  e.damage_type = damage_type;
  if (enemy_type == kEnemyType_Skull) {
    e.total_health = e.hit_points = 1;
    e.sprite.Create(skull_tex_, {10, 13}, 100, 100);
  } else if (enemy_type == kEnemyType_Bug) {
    e.total_health = e.hit_points = 2;
    e.sprite.Create(bug_tex_, {10, 4});
  } else {  // kEnemyType_Tank
    e.total_health = e.hit_points = 6;
    e.sprite.Create(skull_tex_, {10, 13}, 100, 100);
  }
  e.sprite.AutoScale();
  e.sprite.SetVisible(true);
  Vector2 spawn_pos = pos + Vector2(0, e.sprite.GetScale().y / 2);
  e.sprite.SetOffset(spawn_pos);

  e.sprite.SetFrame(enemy_frame_start[enemy_type][damage_type]);
  e.sprite_animator.SetFrames(enemy_frame_count[enemy_type][damage_type],
                              enemy_frame_speed);

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

  image->SetImmutable();
  return image;
}

bool Enemy::CreateRenderResources() {
  Engine& engine = Engine::Get();

  auto skull_image = engine.GetAsset<Image>("enemy_anims_01_frames_ok.png");
  auto bug_image = engine.GetAsset<Image>("enemy_anims_02_frames_ok.png");
  auto target_image = engine.GetAsset<Image>("enemy_target_single_ok.png");
  auto blast_image = engine.GetAsset<Image>("enemy_anims_blast_ok.png");
  if (!skull_image || !bug_image || !target_image || !blast_image)
    return false;

  skull_tex_->Update(skull_image);
  bug_tex_->Update(bug_image);
  target_tex_->Update(target_image);
  blast_tex_->Update(blast_image);

  for (int i = 0; i < kEnemyType_Max; ++i)
    score_tex_[i]->Update(GetScoreImage(GetScore((EnemyType)i)));

  return true;
}
