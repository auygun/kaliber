#include "player.h"

#include <cassert>

#include "../base/log.h"
#include "../engine/engine.h"
#include "../engine/image.h"
#include "../engine/input_event.h"
#include "demo.h"

using base::Vector2;

namespace {

constexpr int wepon_warmup_frame[] = {1, 9};
constexpr int wepon_warmup_frame_count = 4;
constexpr int wepon_cooldown_frame[] = {5, 13};
constexpr int wepon_cooldown_frame_count = 3;
constexpr int wepon_anim_speed = 48;

}  // namespace

bool Player::Initialize() {
  return SetupWeapons();
}

void Player::ContextLost() {
  eng::Engine& engine = eng::Engine::Get();

  auto weapon_image = engine.GetImageAsset("enemy_anims_flare_ok.png");
  auto beam_image = engine.GetImageAsset("enemy_ray_ok.png");

  for (int i = 0; i < 2; ++i) {
    drag_sign_[i].ContextLost();
    drag_sign_[i].Create(weapon_image, {8, 2});
    weapon_[i].ContextLost();
    weapon_[i].Create(weapon_image, {8, 2});
    beam_[i].ContextLost();
    beam_[i].Create(beam_image, {1, 2});
    beam_spark_[i].ContextLost();
    beam_spark_[i].Create(weapon_image, {8, 2});
  }
}

void Player::Update(float delta_time) {
  for (int i = 0; i < 2; ++i) {
    warmup_animator_[i].Update(delta_time);
    cooldown_animator_[i].Update(delta_time);
    beam_animator_[i].Update(delta_time);
    spark_animator_[i].Update(delta_time);
  }

  if (active_weapon_ != kDamageType_Invalid)
    UpdateTarget();
}

void Player::OnInputEvent(std::unique_ptr<eng::InputEvent> event) {
  if (event->GetType() == eng::InputEvent::kDragStart)
    DragStart(event->GetVector(0));
  else if (event->GetType() == eng::InputEvent::kDrag)
    Drag(event->GetVector(0));
  else if (event->GetType() == eng::InputEvent::kDragEnd)
    DragEnd();
  else if (event->GetType() == eng::InputEvent::kDragCancel)
    DragCancel();
}

void Player::Draw(float frame_frac) {
  for (int i = 0; i < 2; ++i) {
    if (drag_sign_[i].IsVisible())
      drag_sign_[i].Draw();
    if (weapon_[i].IsVisible())
      weapon_[i].Draw();
    if (beam_[i].IsVisible())
      beam_[i].Draw();
    if (beam_spark_[i].IsVisible())
      beam_spark_[i].Draw();
  }
}

Vector2 Player::GetWeaponPos(DamageType type) const {
  eng::Engine& engine = eng::Engine::Get();
  return engine.GetScreenSize() /
             Vector2(type == kDamageType_Green ? 4 : -4, -2) +
         Vector2(0, weapon_[type].GetScale().y / 2);
}

Vector2 Player::GetWeaponScale() const {
  return weapon_[0].GetScale();
}

DamageType Player::GetWeaponType(const Vector2& pos) {
  return pos.x < 0 ? kDamageType_Blue : kDamageType_Green;
}

void Player::SetBeamLength(DamageType type, float len) {
  beam_[type].SetOffset({0, 0});
  beam_[type].SetScale({len, beam_[type].GetScale().y});
  beam_[type].PlaceToRightOf(weapon_[type]);
  beam_[type].Translate(weapon_[type].GetScale() * Vector2(-0.5f, 0));
  beam_[type].SetPivot(beam_[type].GetOffset());
  beam_[type].Translate(GetWeaponPos(type));
}

void Player::WarmupWeapon(DamageType type) {
  cooldown_animator_[type].Stop(eng::Animator::kFrames);
  warmup_animator_[type].Play(eng::Animator::kFrames, false);
}

void Player::CooldownWeapon(DamageType type) {
  warmup_animator_[type].Stop(eng::Animator::kFrames);
  cooldown_animator_[type].Play(eng::Animator::kFrames, false);
}

void Player::Fire(DamageType type, Vector2 target_point) {
  eng::Engine& engine = eng::Engine::Get();
  Enemy& enemy = static_cast<Demo*>(engine.GetGame())->GetEnemy();

  Vector2 dir = weapon_[type].GetOffset();
  (enemy.HasTarget(type) ? enemy.GetTargetPos(type) : target_point);
  if (enemy.HasTarget(type)) {
    dir -= enemy.GetTargetPos(type);
  } else {
    dir -= target_point;
    dir.Normalize();
    dir *= engine.GetScreenSize().y * 1.3f;
  }

  float len = dir.Magnitude();
  SetBeamLength(type, len);

  dir.Normalize();
  float cos_theta = dir.DotProduct(Vector2(1, 0));
  float theta = acos(cos_theta) + M_PI;
  beam_[type].SetTheta(theta);
  beam_spark_[type].SetTheta(theta);

  beam_[type].SetColor({1, 1, 1, 1});
  beam_[type].SetVisible(true);
  beam_spark_[type].SetVisible(true);

  spark_animator_[type].SetMovement(-dir * beam_[type].GetScale().x * 0.85f,
                                    18);
  spark_animator_[type].Play(eng::Animator::kMovement, false);
}

bool Player::IsFiring(DamageType type) {
  return beam_animator_[type].IsPlaying(eng::Animator::kBlending) ||
         spark_animator_[type].IsPlaying(eng::Animator::kMovement);
}

bool Player::SetupWeapons() {
  eng::Engine& engine = eng::Engine::Get();

  auto weapon_image = engine.GetImageAsset("enemy_anims_flare_ok.png");
  auto beam_image = engine.GetImageAsset("enemy_ray_ok.png");
  if (!weapon_image || !beam_image)
    return false;

  for (int i = 0; i < 2; ++i) {
    // Setup draw sign.
    drag_sign_[i].Create(weapon_image, {8, 2});
    drag_sign_[i].AutoScale();
    drag_sign_[i].SetFrame(i * 8);

    // Setup weapon.
    weapon_[i].Create(weapon_image, {8, 2});
    weapon_[i].AutoScale();
    weapon_[i].SetVisible(true);
    weapon_[i].SetFrame(wepon_warmup_frame[i]);

    // Setup beam.
    beam_[i].Create(beam_image, {1, 2});
    beam_[i].AutoScale();
    beam_[i].SetFrame(i);
    beam_[i].PlaceToRightOf(weapon_[i]);
    beam_[i].Translate(weapon_[i].GetScale() * Vector2(-0.5f, 0));
    beam_[i].SetPivot(beam_[i].GetOffset());

    // Setup beam spark.
    beam_spark_[i].Create(weapon_image, {8, 2});
    beam_spark_[i].AutoScale();
    beam_spark_[i].SetFrame(i * 8 + 1);
    beam_spark_[i].PlaceToRightOf(weapon_[i]);
    beam_spark_[i].Translate(weapon_[i].GetScale() * Vector2(-0.5f, 0));
    beam_spark_[i].SetPivot(beam_spark_[i].GetOffset());

    // Place parts on the screen.
    Vector2 offset = GetWeaponPos((DamageType)i);
    beam_[i].Translate(offset);
    beam_spark_[i].Translate(offset);
    weapon_[i].Translate(offset);

    // Setup animators.
    weapon_[i].SetFrame(wepon_cooldown_frame[i]);
    cooldown_animator_[i].SetFrames(wepon_cooldown_frame_count,
                                    wepon_anim_speed);
    cooldown_animator_[i].SetEndCallback(
        eng::Animator::kFrames,
        [&, i]() -> void { weapon_[i].SetFrame(wepon_warmup_frame[i]); });
    cooldown_animator_[i].Attach(&weapon_[i]);

    weapon_[i].SetFrame(wepon_warmup_frame[i]);
    warmup_animator_[i].SetFrames(wepon_warmup_frame_count, wepon_anim_speed);
    warmup_animator_[i].SetRotation(M_PI * 2, 0.5f);
    warmup_animator_[i].Attach(&weapon_[i]);
    warmup_animator_[i].Play(eng::Animator::kRotation, true);

    spark_animator_[i].SetEndCallback(
        eng::Animator::kMovement, [&, i]() -> void {
          beam_spark_[i].SetVisible(false);
          beam_animator_[i].Play(eng::Animator::kBlending, false);
          static_cast<Demo*>(engine.GetGame())
              ->GetEnemy()
              .HitTarget((DamageType)i);
        });
    spark_animator_[i].Attach(&beam_spark_[i]);

    beam_animator_[i].SetEndCallback(
        eng::Animator::kBlending,
        [&, i]() -> void { beam_[i].SetVisible(false); });
    beam_animator_[i].SetBlending({1, 1, 1, 0}, 0.16f);
    beam_animator_[i].Attach(&beam_[i]);
  }
  return true;
}

void Player::UpdateTarget() {
  if (IsFiring(active_weapon_))
    return;

  eng::Engine& engine = eng::Engine::Get();
  Demo* game = static_cast<Demo*>(engine.GetGame());

  if (drag_valid_) {
    game->GetEnemy().SelectTarget((DamageType)active_weapon_,
                                  weapon_[active_weapon_].GetOffset(),
                                  drag_end_);
  } else {
    game->GetEnemy().DeselectTarget((DamageType)active_weapon_);
  }
}

void Player::DragStart(const Vector2& pos) {
  drag_start_ = drag_end_ = pos;
  active_weapon_ = GetWeaponType(drag_start_);
  assert(active_weapon_ != kDamageType_Invalid);

  drag_sign_[active_weapon_].SetOffset(drag_start_);
  drag_sign_[active_weapon_].SetVisible(true);
}

void Player::Drag(const Vector2& pos) {
  if (active_weapon_ == kDamageType_Invalid)
    return;

  drag_end_ = pos;
  drag_sign_[active_weapon_].SetOffset(drag_end_);

  if (ValidateDrag()) {
    if (!drag_valid_ && !IsFiring(active_weapon_))
      WarmupWeapon(active_weapon_);
    drag_valid_ = true;
  } else {
    if (drag_valid_ && !IsFiring(active_weapon_))
      CooldownWeapon(active_weapon_);
    drag_valid_ = false;
  }
}

void Player::DragEnd() {
  if (active_weapon_ == kDamageType_Invalid)
    return;

  DamageType type = active_weapon_;
  active_weapon_ = kDamageType_Invalid;
  drag_sign_[type].SetVisible(false);

  if (drag_valid_ && !IsFiring(type)) {
    if (warmup_animator_[type].IsPlaying(eng::Animator::kFrames)) {
      Vector2 target_point = drag_end_;
      warmup_animator_[type].SetEndCallback(
          eng::Animator::kFrames, [&, type, target_point]() -> void {
            warmup_animator_[type].SetEndCallback(eng::Animator::kFrames,
                                                  nullptr);
            CooldownWeapon(type);
            Fire(type, target_point);
          });
    } else {
      CooldownWeapon(type);
      Fire(type, drag_end_);
    }
  }

  drag_valid_ = false;
  drag_start_ = drag_end_ = {0, 0};
}

void Player::DragCancel() {
  if (active_weapon_ == kDamageType_Invalid)
    return;

  DamageType type = active_weapon_;
  active_weapon_ = kDamageType_Invalid;
  drag_sign_[type].SetVisible(false);

  if (drag_valid_ && !IsFiring(type)) {
    if (warmup_animator_[type].IsPlaying(eng::Animator::kFrames)) {
      warmup_animator_[type].SetEndCallback(
          eng::Animator::kFrames, [&, type]() -> void {
            warmup_animator_[type].SetEndCallback(eng::Animator::kFrames,
                                                  nullptr);
            CooldownWeapon(type);
          });
    } else {
      CooldownWeapon(type);
    }
  }

  drag_valid_ = false;
  drag_start_ = drag_end_ = {0, 0};
}

bool Player::ValidateDrag() {
  Vector2 dir = drag_end_ - drag_start_;
  float len = dir.Magnitude();
  dir.Normalize();
  if (len < weapon_[active_weapon_].GetScale().y / 4)
    return false;
  if (dir.DotProduct(Vector2(0, 1)) < 0)
    return false;
  return true;
}
