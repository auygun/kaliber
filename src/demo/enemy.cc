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
  engine::Engine& engine = engine::Engine::Get();
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

void Enemy::Update(float delta_time) {
  engine::Engine& engine = engine::Engine::Get();

  seconds_since_last_spawn_ += delta_time;
  if (seconds_since_last_spawn_ >= 1) {
    seconds_since_last_spawn_ = 0;

    Vector2 s = engine.GetScreenSize();
    float col = (float)(RandomInt() % 5);
    float x = (s.x / 5) / 2 + (s.x / 5) * col - s.x / 2;
    Vector2 pos = {x, s.y / 2};
    float speed = RandomFloat();

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
    it->sprite_frame_animator.Update(delta_time);
    it->target_frame_animator.Update(delta_time);
    it->blast_frame_animator.Update(delta_time);
    it->draw_animator.Update(delta_time);
  }
}

bool Enemy::HasTarget(DamageType damage_type) {
  return GetTarget(damage_type) ? true : false;
}

Vector2 Enemy::GetTargetPos(DamageType damage_type) {
  Unit *target = GetTarget(damage_type);
  if (target)
    return target->sprite.offset() - Vector2(0, target->sprite.scale().y / 2.5f);
  return {0, 0};
}

void Enemy::SelectTarget(DamageType damage_type,
                         const Vector2& weapon_pos,
                         const Vector2& target_pos) {
  Unit* current_enemy = nullptr;
  Unit* best_enemy = nullptr;

  Vector2 beam_dir = (target_pos - weapon_pos).Normalize();
  float closest_dist = std::numeric_limits<float>::max();
  LOG << "begin";
  for (auto& e : enemies_) {
    if (e.hit_points <= 0 || e.marked_for_removal)
      continue;

    if (e.targetted_by_weapon_ == damage_type)
      current_enemy = &e;

    Vector2 weapon_enemy_dir = e.sprite.offset() - weapon_pos;
    float enemy_weapon_dist = weapon_enemy_dir.Magnitude();
    weapon_enemy_dir.Normalize();
    float sin_theta = weapon_enemy_dir.CrossProduct(beam_dir);
    float beam_perpendicular_dist = abs(enemy_weapon_dist * sin_theta);
    if (beam_perpendicular_dist > e.sprite.scale().x)
      continue;

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
    current_enemy->target_frame_animator.Stop();
  }

  if (best_enemy) {
    best_enemy->targetted_by_weapon_ = damage_type;
    best_enemy->target.SetVisible(true);
    if (damage_type == kDamageType_Green)
      best_enemy->target_frame_animator.SetFrameRange(0 ,6, 5);
    else
      best_enemy->target_frame_animator.SetFrameRange(6 ,12, 11);
    best_enemy->target_frame_animator.Play(false, true);
  }
}

void Enemy::DeselectTarget(DamageType damage_type) {
  Unit *target = GetTarget(damage_type);
  if (target) {
    target->targetted_by_weapon_ = kDamageType_Invalid;
    target->target.SetVisible(false);
    target->target_frame_animator.Stop();
  }
}

void Enemy::HitTarget(DamageType damage_type) {
  Unit *target = GetTarget(damage_type);
  if (!target || target->damage_type != damage_type) {
    if (target)
      target->target.SetVisible(false);
    return;
  }
  if (--target->hit_points <= 0) {
    target->sprite.SetVisible(false);
    target->blast.SetVisible(true);
    target->blast_frame_animator.Play(false, true);
    target->blast_frame_animator.SetCallback(5, [target]()->void {
      target->marked_for_removal = true;
    });
  } else {
    target->targetted_by_weapon_ = kDamageType_Invalid;
    target->blast.SetVisible(true);
    target->target_frame_animator.Stop();
    target->blast_frame_animator.Play(false, true);
    target->blast_frame_animator.SetCallback(5, [target]()->void {
      target->target.SetVisible(false);
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

  engine::Engine& engine = engine::Engine::Get();
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
  e.sprite.Scale(2);
  e.sprite.SetVisible(true);
  e.sprite.SetOffset(pos);
  engine.AddDrawable(&e.sprite);

  e.sprite_frame_animator.AttachFrameController(&e.sprite);
  if (damage_type == kDamageType_Green) {
    if (unit_type == kUnitType_Skull)
      e.sprite_frame_animator.SetFrameRange(0 ,5, 4);
    else
      e.sprite_frame_animator.SetFrameRange(13 ,19, 18);
  } else {
    if (unit_type == kUnitType_Skull)
      e.sprite_frame_animator.SetFrameRange(30 ,35, 34);
    else
      e.sprite_frame_animator.SetFrameRange(34 ,39, 38);
  }
  e.sprite_frame_animator.Play(true, true);

  e.target.Create(target_frames_, {6, 2});
  e.target.Scale(2);
  e.target.SetOffset(pos);
  engine.AddDrawable(&e.target);

  e.blast.Create(blast_frames_, {6, 2});
  e.blast.Scale(2);
  e.blast.SetOffset(pos);
  engine.AddDrawable(&e.blast);

  e.target_frame_animator.AttachFrameController(&e.target);
  e.target_frame_animator.SetSpeed(0.05f);

  e.blast_frame_animator.AttachFrameController(&e.blast);
  e.blast_frame_animator.SetSpeed(0.05f);
  if (damage_type == kDamageType_Green)
    e.blast_frame_animator.SetFrameRange(0 ,6, 5);
  else
    e.blast_frame_animator.SetFrameRange(6 ,12, 11);

  float max_distance = engine.GetScreenSize().y / 2 +
      abs(game->GetPlayer().GetWeaponPos(kDamageType_Green).y) -
      game->GetPlayer().GetWeaponScale(kDamageType_Green).y;

  e.draw_animator.AttachDrawable(&e.sprite);
  e.draw_animator.AttachDrawable(&e.target);
  e.draw_animator.AttachDrawable(&e.blast);
  e.draw_animator.SetMovement({0, -1}, max_distance);
  e.draw_animator.SetSpeed(0.006f);
  e.draw_animator.SetCallback([&]()->void {
    e.sprite_frame_animator.Stop();
    e.sprite.SetVisible(false);
    e.marked_for_removal = true;
  });
  e.draw_animator.Play(false);
}

Enemy::Unit* Enemy::GetTarget(DamageType damage_type) {
  for (auto& e : enemies_) {
    if (e.targetted_by_weapon_ == damage_type && e.hit_points > 0 &&
       !e.marked_for_removal)
      return &e;
  }
  return nullptr;
}
