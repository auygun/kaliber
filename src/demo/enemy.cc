#include "enemy.h"
#include "demo.h"
#include "../base/log.h"
#include "../base/random.h"
#include "../engine/image.h"
#include "../engine/font.h"
#include "../engine/engine.h"
#include <memory>
#include <limits>
#include <cassert>

namespace {

constexpr int enemy_frame_start[][3] = {{ 0, 50,  -1},
                                        {13, 33,  -1},
                                        {-1, -1, 100}};
constexpr int enemy_frame_count[][3] = {{ 7,  7, -1},
                                        { 6,  6, -1},
                                        {-1, -1,  7}};
constexpr int enemy_frame_speed = 12;

constexpr int enemy_scores[] = {100, 150, 200};

constexpr float score_bg_color[4] = {1, 1, 1, 0};

} // namespace

bool Enemy::Initialize() {
  eng::Engine& engine = eng::Engine::Get();
  skull_frames_ = engine.GetImageAsset("enemy_anims_01_frames_ok.png");
  tank_frames_ = engine.GetImageAsset("enemy_anims_01_frames_ok.png");
  bug_frames_ = engine.GetImageAsset("enemy_anims_02_frames_ok.png");
  target_frames_ = engine.GetImageAsset("enemy_target_single_ok.png");
  blast_frames_ = engine.GetImageAsset("enemy_anims_blast_ok.png");
  font_ = engine.GetFontAsset("PixelCaps!.ttf");
  return true;
}

void Enemy::ContextLost() {
  for (auto& e : enemies_) {
    e.sprite.ContextLost();
    if (e.unit_type == kUnitType_Skull)
      e.sprite.Create(skull_frames_, {10, 13}, 100, 100);
    else if (e.unit_type == kUnitType_Bug)
      e.sprite.Create(bug_frames_, {10, 4});
    else // kUnitType_Tank
      e.sprite.Create(tank_frames_, {10, 13}, 100, 100);
    e.target.ContextLost();
    e.target.Create(target_frames_, {6, 2});
    e.blast.ContextLost();
    e.blast.Create(blast_frames_, {6, 2});
  }
}

void Enemy::Update(float delta_time) {
  eng::Engine& engine = eng::Engine::Get();

  seconds_since_last_spawn_ += delta_time;
  if (seconds_since_last_spawn_ >= 1) {
    seconds_since_last_spawn_ = 0;

    UnitType unit_type = (RandomInt() % 12) == 0 ? kUnitType_Tank :
        ((RandomInt() % 5) == 0 ? kUnitType_Bug : kUnitType_Skull);
    DamageType damage_type = unit_type == kUnitType_Tank ? kDamageType_Any :
        (DamageType)(RandomInt() % kDamageType_Any);

    Vector2 s = engine.GetScreenSize();
    float col = (float)(RandomInt() % 4);
    float x = (s.x / 4) / 2 + (s.x / 4) * col - s.x / 2;
    Vector2 pos = {x, s.y / 2};
    float speed = unit_type == kUnitType_Tank ? 0.1f :
        ((RandomInt() % 4) == 0 ? 0.65f : 0.4f);

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
    it->health_animator.Update(delta_time);
    it->score_animator.Update(delta_time);
    it->movement_animator.Update(delta_time);
  }
}

void Enemy::Draw(float frame_frac) {
  for (auto& e : enemies_) {
    if (e.sprite.IsVisible())
      e.sprite.Draw();
    if (e.target.IsVisible())
      e.target.Draw();
    if (e.blast.IsVisible())
      e.blast.Draw();
    if (e.health_base.IsVisible())
      e.health_base.Draw();
    if (e.health_bar.IsVisible())
      e.health_bar.Draw();
    if (e.score.IsVisible())
      e.score.Draw();
  }
}

bool Enemy::HasTarget(DamageType damage_type) {
  assert(damage_type > kDamageType_Invalid && damage_type < kDamageType_Any);

  return GetTarget(damage_type) ? true : false;
}

Vector2 Enemy::GetTargetPos(DamageType damage_type) {
  assert(damage_type > kDamageType_Invalid && damage_type < kDamageType_Any);

  Unit *target = GetTarget(damage_type);
  if (target)
    return target->sprite.GetOffset() - Vector2(0, target->sprite.GetScale().y / 2.5f);
  return {0, 0};
}

void Enemy::SelectTarget(DamageType damage_type,
                         const Vector2& weapon_pos,
                         const Vector2& target_pos) {
  assert(damage_type > kDamageType_Invalid && damage_type < kDamageType_Any);

  Unit* current_enemy = nullptr;
  Unit* best_enemy = nullptr;

  Vector2 beam_dir = (target_pos - weapon_pos).Normalize();
  float closest_dist = std::numeric_limits<float>::max();
  for (auto& e : enemies_) {
    if (e.hit_points <= 0 || e.marked_for_removal)
      continue;

    if (e.targetted_by_weapon_ == damage_type)
      current_enemy = &e;

    if (!Intersection(e.sprite.GetOffset(), e.sprite.GetScale() * 1.2f,
        weapon_pos, beam_dir))
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
    if (damage_type == kDamageType_Green) {
      best_enemy->target.SetFrame(0);
      best_enemy->target_animator.SetFrames(6, 28);
    } else {
      best_enemy->target.SetFrame(6);
      best_enemy->target_animator.SetFrames(6, 28);
    }
    best_enemy->target_animator.Play(eng::Animator::kFrames, false);
  }
}

void Enemy::DeselectTarget(DamageType damage_type) {
  assert(damage_type > kDamageType_Invalid && damage_type < kDamageType_Any);

  Unit *target = GetTarget(damage_type);
  if (target) {
    target->targetted_by_weapon_ = kDamageType_Invalid;
    target->target.SetVisible(false);
    target->target_animator.Stop(eng::Animator::kAllAnimations);
  }
}

void Enemy::HitTarget(DamageType damage_type) {
  assert(damage_type > kDamageType_Invalid && damage_type < kDamageType_Any);

  Unit *target = GetTarget(damage_type);

  if (target) {
    target->target.SetVisible(false);
    target->target_animator.Stop(eng::Animator::kAllAnimations);
  }

  if (!target || (target->damage_type != kDamageType_Any &&
      target->damage_type != damage_type))
    return;

  target->blast.SetVisible(true);
  target->blast_animator.Play(eng::Animator::kFrames, false);

  if (--target->hit_points <= 0) {
    ++num_enemies_killed_;

    target->sprite.SetVisible(false);
    target->health_base.SetVisible(false);
    target->health_bar.SetVisible(false);
    target->score.SetVisible(true);

    target->score_animator.Play(eng::Animator::kAllAnimations, false);
    target->movement_animator.Pause(eng::Animator::kMovement);

    eng::Engine& engine = eng::Engine::Get();
    Demo* game = static_cast<Demo*>(engine.GetGame());
    game->AddScore(GetScore(target->unit_type));
  } else {
    target->targetted_by_weapon_ = kDamageType_Invalid;

    Vector2 s = target->sprite.GetScale() * Vector2(0.6f, 0.01f);
    s.x *= (float)target->hit_points / (float)target->total_health;
    float t = (s.x - target->health_bar.GetScale().x) / 2;
    target->health_bar.SetScale(s);
    target->health_bar.Translate({t, 0});

    target->health_base.SetVisible(true);
    target->health_bar.SetVisible(true);

    target->health_animator.Stop(eng::Animator::kTimer |
        eng::Animator::kBlending);
    target->health_animator.Play(eng::Animator::kTimer, false);
  }
}

void Enemy::Spawn(UnitType unit_type,
                  DamageType damage_type,
                  const Vector2& pos,
                  float speed) {
  assert(unit_type > kUnitType_Invalid && unit_type < kUnitType_Max);
  assert(damage_type > kDamageType_Invalid && damage_type < kDamageType_Max);
  assert(unit_type == kUnitType_Tank && damage_type == kDamageType_Any ||
         unit_type != kUnitType_Tank && damage_type != kDamageType_Any);

  eng::Engine& engine = eng::Engine::Get();
  Demo* game = static_cast<Demo*>(engine.GetGame());

  auto& e = enemies_.emplace_back();
  e.unit_type = unit_type;
  e.damage_type = damage_type;
  if (unit_type == kUnitType_Skull) {
    e.total_health = e.hit_points = 1;
    e.sprite.Create(skull_frames_, {10, 13}, 100, 100);
  } else if (unit_type == kUnitType_Bug) {
    e.total_health = e.hit_points = 2;
    e.sprite.Create(bug_frames_, {10, 4});
  } else { // kUnitType_Tank
    e.total_health = e.hit_points = 10;
    e.sprite.Create(tank_frames_, {10, 13}, 100, 100);
  }
  e.sprite.AutoScale();
  e.sprite.SetVisible(true);
  Vector2 spawn_pos = pos + Vector2(0, e.sprite.GetScale().y /2);
  e.sprite.SetOffset(spawn_pos);

  e.sprite.SetFrame(enemy_frame_start[unit_type][damage_type]);
  e.sprite_animator.SetFrames(enemy_frame_count[unit_type][damage_type],
      enemy_frame_speed);

  e.sprite_animator.Attach(&e.sprite);
  e.sprite_animator.Play(eng::Animator::kFrames, true);

  e.target.Create(target_frames_, {6, 2});
  e.target.AutoScale();
  e.target.SetOffset(spawn_pos);

  e.blast.Create(blast_frames_, {6, 2});
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

  auto image = std::make_shared<eng::Image>();
  image->Create(e.sprite.frame_width(), e.sprite.frame_height());
  image->Clear(score_bg_color);
  std::string text = std::to_string(GetScore(e.unit_type));
  int w, h;
  font_->CalculateBoundingBox(text.c_str(), w, h);
  int x = (image->GetOriginalWidth() - w) / 2;
  int y = (image->GetOriginalHeight() - h) / 2;
  font_->Print(x, y, text.c_str(), image->GetBuffer(), image->GetWidth());
  image->SetImmutable();

  e.score.Create(image);
  e.score.AutoScale();
  e.score.SetColor({1, 1, 1, 1});
  e.score.SetOffset(spawn_pos);

  e.target_animator.Attach(&e.target);

  e.blast_animator.SetEndCallback(eng::Animator::kFrames, [&]()->void {
    e.blast.SetVisible(false);
  });
  if (damage_type == kDamageType_Green) {
    e.blast.SetFrame(0);
    e.blast_animator.SetFrames(6, 28);
  } else {
    e.blast.SetFrame(6);
    e.blast_animator.SetFrames(6 ,28);
  }
  e.blast_animator.Attach(&e.blast);

  e.health_animator.SetEndCallback(eng::Animator::kTimer, [&]()->void {
    e.health_animator.SetBlending({1, 1, 1, 0}, 0.5f);
    e.health_animator.Play(eng::Animator::kBlending, false);
  });
  e.health_animator.SetEndCallback(eng::Animator::kBlending, [&]()->void {
    e.health_base.SetVisible(false);
    e.health_bar.SetVisible(false);
  });
  e.health_animator.SetTimer(1.0f);
  e.health_animator.Attach(&e.health_base);
  e.health_animator.Attach(&e.health_bar);

  e.score_animator.SetMovement({0, 2}, 1.0f);
  e.score_animator.SetBlending({1, 1, 1, 0}, 0.7f);
  e.score_animator.SetEndCallback(eng::Animator::kBlending, [&]()->void {
    e.score_animator.Stop(eng::Animator::kAllAnimations);
    e.score.SetVisible(false);
    e.marked_for_removal = true;
  });
  e.score_animator.Attach(&e.score);

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
  e.movement_animator.Attach(&e.health_base);
  e.movement_animator.Attach(&e.health_bar);
  e.movement_animator.Attach(&e.score);
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

int Enemy::GetScore(UnitType unit_type) {
  assert(unit_type > kUnitType_Invalid && unit_type < kUnitType_Max);
  return enemy_scores[unit_type];
}
