#include "enemy.h"
#include "demo.h"
#include "../base/log.h"
#include "../base/random.h"
#include "../engine/asset_manager/image.h"
#include "../engine/engine.h"
#include <memory>
#include <limits>

bool Enemy::Initialize() {
  engine::Engine& engine = engine::Engine::Get();
  enemy_frames_ = engine.GetAssetManager().GetImage(
      "enemy_anims_01_frames_ok.png");
  target_frames_ = engine.GetAssetManager().GetImage(
      "enemy_target_single_ok.png");
  return true;
}

void Enemy::Update(float delta_time) {
  engine::Engine& engine = engine::Engine::Get();

  seconds_since_last_spawn_ += delta_time;
  if (seconds_since_last_spawn_ >= 3) {
    seconds_since_last_spawn_ = 0;
    Type type = (Type)(RandomInt() % 2);
    Vector2 s = engine.GetScreenSize();
    float col = (float)(RandomInt() % 5);
    float x = (s.x / 5) / 2 + (s.x / 5) * col - s.x / 2;
    Vector2 pos = {x, s.y / 2};
    float speed = RandomFloat();
    Spawn(type, pos, speed);
  }

  for (auto it = enemies_.begin(); it != enemies_.end(); ++ it) {
    if (!it->alive) {
      it = enemies_.erase(it);
      continue;
    }
    it->sprite_frame_animator.Update(delta_time);
    it->target_frame_animator.Update(delta_time);
    it->draw_animator.Update(delta_time);
  }
}

void Enemy::SelectTarget(const Vector2& weapon_pos, const Vector2& target_pos) {
  EnemyTraits* current_enemy = nullptr;
  EnemyTraits* best_enemy = nullptr;

  Vector2 beam_dir = (target_pos - weapon_pos).Normalize();
  float closest_dist = std::numeric_limits<float>::max();
  LOG << "begin";
  for (auto& e : enemies_) {
    if (e.targetted)
      current_enemy = &e;

    float target_enemy_dist = (e.sprite.offset() - target_pos).Magnitude();
    if (e.sprite.offset().y > target_pos.y &&
        target_enemy_dist > e.sprite.scale().y * 3)
      continue;

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
    current_enemy->targetted = false;
    current_enemy->target.SetVisible(false);
    current_enemy->target_frame_animator.Stop();
  }

  if (best_enemy) {
    best_enemy->targetted = true;
    best_enemy->target.SetVisible(true);
    best_enemy->target_frame_animator.Play(false, true);
  }
}

void Enemy::Spawn(Type type, const Vector2& pos, float speed) {
  engine::Engine& engine = engine::Engine::Get();
  Demo* game = static_cast<Demo*>(engine.GetGame());

  auto& e = enemies_.emplace_back();
  e.type = type;
  e.sprite.Create(enemy_frames_, {10, 6});
  e.sprite.Scale(2);
  e.sprite.SetVisible(true);
  e.sprite.SetOffset(pos);
  engine.AddDrawable(&e.sprite);

  e.sprite_frame_animator.AttachFrameController(&e.sprite);
  if (type == kGreem)
    e.sprite_frame_animator.SetFrameRange(0 ,21, 20);
  else
    e.sprite_frame_animator.SetFrameRange(30 ,51, 50);
  e.sprite_frame_animator.Play(true, true);

  e.target.Create(target_frames_, {6, 2});
  e.target.Scale(2);
  e.target.SetOffset(pos);
  engine.AddDrawable(&e.target);

  e.target_frame_animator.AttachFrameController(&e.target);
  if (type == kGreem)
    e.target_frame_animator.SetFrameRange(0 ,6, 5);
  else
    e.target_frame_animator.SetFrameRange(6 ,12, 11);

  float max_distance = engine.GetScreenSize().y / 2 +
      abs(game->GetPlayer().GetWeaponPos(0).y) -
      game->GetPlayer().GetWeaponScale(0).y;

  e.draw_animator.AttachDrawable(&e.sprite);
  e.draw_animator.AttachDrawable(&e.target);
  e.draw_animator.SetMovement({0, -1}, max_distance);
  e.draw_animator.SetCallback([&]()->void {
    e.sprite_frame_animator.Stop();
    e.sprite.SetVisible(false);
    e.alive = false;
  });
  e.draw_animator.Play(false);
}
