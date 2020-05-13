#include "enemy.h"
#include "demo.h"
#include "../base/log.h"
#include "../base/random.h"
#include "../engine/asset_manager/image.h"
#include "../engine/engine.h"
#include <memory>
#include <limits>
#include <cassert>

bool Enemy::Initialize() {
  eng::Engine& engine = eng::Engine::Get();
  skull_frames_ = engine.GetAssetManager().GetImage(
      "enemy_anims_01_frames_ok.png");
  bug_frames_ = engine.GetAssetManager().GetImage(
      "enemy_anims_02_frames_ok.png");
  target_frames_ = engine.GetAssetManager().GetImage(
      "enemy_target_single_ok.png");
  blast_frames_ = engine.GetAssetManager().GetImage(
      "enemy_anims_blast_ok.png");
  return true;
}

void Enemy::ContextLost() {
  for (auto& e : enemies_) {
    if (e.marked_for_removal)
      continue;

    if (e.unit_type == kUnitType_Skull)
      e.sprite.Create(skull_frames_, {10, 6});
    else
      e.sprite.Create(bug_frames_, {10, 4});
    e.target.Create(target_frames_, {6, 2});
    e.blast.Create(blast_frames_, {6, 2});
  }
}

void Enemy::Update(float delta_time) {
  eng::Engine& engine = eng::Engine::Get();

  seconds_since_last_spawn_ += delta_time;
  if (seconds_since_last_spawn_ >= 1) {
    seconds_since_last_spawn_ = 0;

    Vector2 s = engine.GetScreenSize();
    float col = (float)(RandomInt() % 4);
    float x = (s.x / 4) / 2 + (s.x / 4) * col - s.x / 2;
    Vector2 pos = {x, s.y / 2};
    float speed = (RandomInt() % 4) == 0 ? 0.65f : 0.4f;

    UnitType unit_type = (RandomInt() % 5) == 0 ?
        kUnitType_Bug : kUnitType_Skull;
    DamageType damage_type = (DamageType)(RandomInt() % kDamageType_Max);
    Spawn(unit_type, damage_type, pos, speed);
  }

  for (auto it = enemies_.begin(); it != enemies_.end(); ++ it) {
    if (it->marked_for_removal) {
      it = enemies_.erase(it);
      continue;
    }
    it->sprite_animator.Update(delta_time);
    it->target_animator.Update(delta_time);
    it->blast_animator.Update(delta_time);
    it->movement_animator.Update(delta_time);
  }
}

bool Enemy::HasTarget(DamageType damage_type) {
  return GetTarget(damage_type) ? true : false;
}

Vector2 Enemy::GetTargetPos(DamageType damage_type) {
  Unit *target = GetTarget(damage_type);
  if (target)
    return target->sprite.GetOffset() - Vector2(0, target->sprite.GetScale().y / 2.5f);
  return {0, 0};
}

void Enemy::SelectTarget(DamageType damage_type,
                         const Vector2& weapon_pos,
                         const Vector2& target_pos) {
  Unit* current_enemy = nullptr;
  Unit* best_enemy = nullptr;

  Vector2 beam_dir = (target_pos - weapon_pos).Normalize();
  float closest_dist = std::numeric_limits<float>::max();
  for (auto& e : enemies_) {
    if (e.hit_points <= 0 || e.marked_for_removal)
      continue;

    if (e.targetted_by_weapon_ == damage_type)
      current_enemy = &e;

    if (!Intersection(e.sprite.GetOffset(), e.sprite.GetScale(), weapon_pos,
        beam_dir))
      continue;

    Vector2 weapon_enemy_dir = e.sprite.GetOffset() - weapon_pos;
    float enemy_weapon_dist = weapon_enemy_dir.Magnitude();
    if (closest_dist > enemy_weapon_dist) {
      closest_dist = enemy_weapon_dist;
      best_enemy = &e;
    }
  }

  if (best_enemy == current_enemy)
    return;

  if (current_enemy) {
    current_enemy->targetted_by_weapon_ = kDamageType_Invalid;
    current_enemy->target.SetVisible(false);
    current_enemy->target_animator.Stop(eng::Animator::kAllAnimations);
  }

  if (best_enemy) {
    best_enemy->targetted_by_weapon_ = damage_type;
    best_enemy->target.SetVisible(true);
    best_enemy->target_animator.Play(eng::Animator::kFrames, false);
  }
}

void Enemy::DeselectTarget(DamageType damage_type) {
  Unit *target = GetTarget(damage_type);
  if (target) {
    target->targetted_by_weapon_ = kDamageType_Invalid;
    target->target.SetVisible(false);
    target->target_animator.Stop(eng::Animator::kAllAnimations);
  }
}

void Enemy::HitTarget(DamageType damage_type) {
  Unit *target = GetTarget(damage_type);

  if (target) {
    target->target.SetVisible(false);
    target->target_animator.Stop(eng::Animator::kAllAnimations);
  }

  if (!target || target->damage_type != damage_type)
    return;

  target->blast.SetVisible(true);
  target->blast_animator.Play(eng::Animator::kFrames, false);

  if (--target->hit_points <= 0) {
    target->sprite.SetVisible(false);
    target->blast_animator.SetEndCallback(eng::Animator::kFrames, [target]()->void {
      target->sprite.SetVisible(false);
      target->blast.SetVisible(false);
      target->marked_for_removal = true;
    });

    eng::Engine& engine = eng::Engine::Get();
    Demo* game = static_cast<Demo*>(engine.GetGame());
    game->AddScore(target->unit_type == kUnitType_Skull ? 100 : 200);
  } else {
    target->targetted_by_weapon_ = kDamageType_Invalid;
    target->blast_animator.SetEndCallback(eng::Animator::kFrames, [target]()->void {
      target->blast.SetVisible(false);
    });
  }
}

void Enemy::Spawn(UnitType unit_type,
                  DamageType damage_type,
                  const Vector2& pos,
                  float speed) {
  assert(unit_type > kUnitType_Invalid && unit_type < kUnitType_Max);
  assert(damage_type > kDamageType_Invalid && damage_type < kDamageType_Max);

  eng::Engine& engine = eng::Engine::Get();
  Demo* game = static_cast<Demo*>(engine.GetGame());

  auto& e = enemies_.emplace_back();
  e.unit_type = unit_type;
  e.damage_type = damage_type;
  if (unit_type == kUnitType_Skull) {
    e.hit_points = 1;
    e.sprite.Create(skull_frames_, {10, 6});
  } else {
    e.hit_points = 2;
    e.sprite.Create(bug_frames_, {10, 4});
  }
  e.sprite.AutoScale();
  e.sprite.SetVisible(true);
  Vector2 spawn_pos = pos + Vector2(0, e.sprite.GetScale().y /2);
  e.sprite.SetOffset(spawn_pos);
  engine.AddDrawable(&e.sprite);

  if (damage_type == kDamageType_Green) {
    if (unit_type == kUnitType_Skull) {
      e.sprite.SetFrame(0);
      e.sprite_animator.SetFrames(6, 12);
    } else {
      e.sprite.SetFrame(13);
      e.sprite_animator.SetFrames(6, 12);
    }
  } else {
    if (unit_type == kUnitType_Skull) {
      e.sprite.SetFrame(30);
      e.sprite_animator.SetFrames(6, 12);
    } else {
      e.sprite.SetFrame(33);
      e.sprite_animator.SetFrames(6, 12);
    }
  }
  e.sprite_animator.Attach(&e.sprite);
  e.sprite_animator.Play(eng::Animator::kFrames, true);

  e.target.Create(target_frames_, {6, 2});
  e.target.AutoScale();
  e.target.SetOffset(spawn_pos);
  engine.AddDrawable(&e.target);

  e.blast.Create(blast_frames_, {6, 2});
  e.blast.AutoScale();
  e.blast.SetOffset(spawn_pos);
  engine.AddDrawable(&e.blast);

  if (damage_type == kDamageType_Green) {
    e.target.SetFrame(0);
    e.target_animator.SetFrames(6, 28);
  } else {
    e.target.SetFrame(6);
    e.target_animator.SetFrames(6, 28);
  }
  e.target_animator.Attach(&e.target);

  if (damage_type == kDamageType_Green) {
    e.blast.SetFrame(0);
    e.blast_animator.SetFrames(6, 28);
  } else {
    e.blast.SetFrame(6);
    e.blast_animator.SetFrames(6 ,28);
  }
  e.blast_animator.Attach(&e.blast);

  float max_distance = engine.GetScreenSize().y -
      game->GetPlayer().GetWeaponScale().y;

  e.movement_animator.SetMovement({0, -max_distance}, speed);
  e.movement_animator.SetEndCallback(eng::Animator::kMovement, [&]()->void {
    e.sprite_animator.Stop(eng::Animator::kAllAnimations);
    e.sprite.SetVisible(false);
    e.target.SetVisible(false);
    e.blast.SetVisible(false);
    e.marked_for_removal = true;
  });
  e.movement_animator.Attach(&e.sprite);
  e.movement_animator.Attach(&e.target);
  e.movement_animator.Attach(&e.blast);
  e.movement_animator.Play(eng::Animator::kMovement, false);
}

Enemy::Unit* Enemy::GetTarget(DamageType damage_type) {
  for (auto& e : enemies_) {
    if (e.targetted_by_weapon_ == damage_type && e.hit_points > 0 &&
       !e.marked_for_removal)
      return &e;
  }
  return nullptr;
}
