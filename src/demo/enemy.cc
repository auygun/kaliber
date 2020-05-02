#include "enemy.h"
#include "../base/log.h"
#include "../base/random.h"
#include "../engine/asset_manager/image.h"
#include "../engine/engine.h"
#include <memory>

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

void Enemy::TryTarget(const Vector2& weapon_pos, const Vector2& target_pos) {
  Vector2 beam_dir = (target_pos - weapon_pos).Normalize();

  float smallest_cos_ang = 0;
  EnemyTraits* current_enemy = nullptr;
  EnemyTraits* best_enemy = nullptr;
  for (auto& e : enemies_) {
    if (e.targetted)
      current_enemy = &e;
    if ((e.sprite.offset() - target_pos).Magnitude() > e.sprite.scale().y * 2)
      continue;
    Vector2 enemy_dir = (e.sprite.offset() - weapon_pos).Normalize();
    float cos_ang = enemy_dir.DotProduct(beam_dir);
    LOG << cos_ang;
    if (cos_ang > smallest_cos_ang) {
      smallest_cos_ang = cos_ang;
      best_enemy = &e;
    }
  }

  if (current_enemy && (best_enemy != current_enemy || smallest_cos_ang < 0.98f)) {
    current_enemy->targetted = false;
    current_enemy->target.SetVisible(false);
    current_enemy->target_frame_animator.Stop();
  }

  if (best_enemy && best_enemy != current_enemy && smallest_cos_ang >= 0.98f) {
    best_enemy->targetted = true;
    best_enemy->target.SetVisible(true);
    best_enemy->target_frame_animator.Play(false, true);
  }
}

void Enemy::Spawn(Type type, const Vector2& pos, float speed) {
  engine::Engine& engine = engine::Engine::Get();

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

  e.draw_animator.AttachDrawable(&e.sprite);
  e.draw_animator.AttachDrawable(&e.target);
  e.draw_animator.SetMovement({0, -1}, engine.GetScreenSize().y);
  e.draw_animator.SetCallback([&]()->void {
    e.sprite_frame_animator.Stop();
    e.sprite.SetVisible(false);
    e.alive = false;
  });
  e.draw_animator.Play(false);
}
