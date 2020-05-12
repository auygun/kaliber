#include "player.h"
#include "demo.h"
#include "../base/log.h"
#include "../engine/asset_manager/image.h"
#include "../engine/engine.h"
#include "../engine/input_event.h"
#include <math.h>
#include <memory>
#include <cassert>

namespace {

constexpr int wepon_fire_frame[] = {1, 9};
constexpr int wepon_fire_frame_count = 4;
constexpr int wepon_cooldown_frame[] = {5, 13};
constexpr int wepon_cooldown_frame_count = 3;
constexpr int wepon_anim_speed = 48;

} // namespace

bool Player::Initialize() {
  SetupWeapons();
  return true;
}

void Player::ContextLost() {
  eng::Engine& engine = eng::Engine::Get();

  auto weapon_image =
      engine.GetAssetManager().GetImage("enemy_anims_flare_ok.png");
  auto beam_image =
      engine.GetAssetManager().GetImage("enemy_ray_ok.png");

  for (int i = 0; i < 2; ++i) {
    drag_sign_[i].Create(weapon_image, {8, 2});
    weapon_[i].Create(weapon_image, {8, 2});
    beam_[i].Create(beam_image, {1, 2});
    beam_spark_[i].Create(weapon_image, {8, 2});
  }
}

void Player::Update(float delta_time) {
  for (int i = 0; i < 2; ++i) {
    weapon_animator_[i].Update(delta_time);
    beam_animator_[i].Update(delta_time);
    spark_animator_[i].Update(delta_time);
  }

  if (active_weapon_ != kDamageType_Invalid)
    SelectTarget();
}

void Player::OnInputEvent(std::unique_ptr<eng::InputEvent> event) {
  if (event->GetEventType() == eng::InputEvent::kDragStart)
    DragStart(event->GetEventVector(0));
  else if (event->GetEventType() == eng::InputEvent::kDrag)
    Drag(event->GetEventVector(0));
  else if (event->GetEventType() == eng::InputEvent::kDragEnd)
    DragEnd();
}

Vector2 Player::GetWeaponPos(DamageType type) const {
  eng::Engine& engine = eng::Engine::Get();
  return engine.GetScreenSize() /
      Vector2(type == kDamageType_Green ? 4 : -4 , -2) +
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

void Player::Fire(DamageType type) {
  eng::Engine& engine = eng::Engine::Get();
  Enemy &enemy = static_cast<Demo*>(engine.GetGame())->GetEnemy();

  Vector2 dir = weapon_[type].GetOffset();
      (enemy.HasTarget(type) ? enemy.GetTargetPos(type) : drag_end_);
  if (enemy.HasTarget(type)) {
    dir -= enemy.GetTargetPos(type);
  } else {
    dir -= drag_end_;
    dir.Normalize();
    dir *= engine.GetScreenSize().y * 1.3f;
  }

  float len = dir.Magnitude();
  SetBeamLength(type, len);

  dir.Normalize();
  float cos_angle = dir.DotProduct(Vector2(1 ,0));
  float angle = acos(cos_angle) + M_PI;
  beam_[type].Rotate(angle);
  beam_spark_[type].Rotate(angle);

  spark_animator_[type].SetMovement(-dir * beam_[type].GetScale().x * 0.85f, 18);
  weapon_animator_[type].SetFrames(wepon_fire_frame_count, wepon_anim_speed);
  weapon_animator_[type].SetEndCallback(eng::Animator::kFrames, weapon_animator_cb_[type]);
  weapon_animator_[type].Play(eng::Animator::kFrames, false);
}

bool Player::IsFiring(DamageType type) {
  return weapon_animator_[type].IsPlaying(eng::Animator::kFrames) ||
      beam_animator_[type].IsPlaying(eng::Animator::kBlending) ||
      spark_animator_[type].IsPlaying(eng::Animator::kMovement);
}

void Player::SetupWeapons() {
  eng::Engine& engine = eng::Engine::Get();

  auto weapon_image =
      engine.GetAssetManager().GetImage("enemy_anims_flare_ok.png");
  auto beam_image =
      engine.GetAssetManager().GetImage("enemy_ray_ok.png");

  for (int i = 0; i < 2; ++i) {
    // Setup draw sign.
    drag_sign_[i].Create(weapon_image, {8, 2});
    drag_sign_[i].AutoScale();
    drag_sign_[i].SetFrame(i * 8);
    engine.AddDrawable(&drag_sign_[i]);

    // Setup weapon.
    weapon_[i].Create(weapon_image, {8, 2});
    weapon_[i].AutoScale();
    weapon_[i].SetVisible(true);
    weapon_[i].SetFrame(wepon_fire_frame[i]);
    engine.AddDrawable(&weapon_[i]);

    // Setup beam.
    beam_[i].Create(beam_image, {1, 2});
    beam_[i].AutoScale();
    beam_[i].SetFrame(i);
    beam_[i].PlaceToRightOf(weapon_[i]);
    beam_[i].Translate(weapon_[i].GetScale() * Vector2(-0.5f, 0));
    beam_[i].SetPivot(beam_[i].GetOffset());
    engine.AddDrawable(&beam_[i]);

    // Setup beam spark.
    beam_spark_[i].Create(weapon_image, {8, 2});
    beam_spark_[i].AutoScale();
    beam_spark_[i].SetFrame(i * 8 + 1);
    beam_spark_[i].PlaceToRightOf(weapon_[i]);
    beam_spark_[i].Translate(weapon_[i].GetScale() * Vector2(-0.5f, 0));
    beam_spark_[i].SetPivot(beam_spark_[i].GetOffset());
    engine.AddDrawable(&beam_spark_[i]);

    // Place parts on the screen.
    Vector2 offset = GetWeaponPos((DamageType)i);
    beam_[i].Translate(offset);
    beam_spark_[i].Translate(offset);
    weapon_[i].Translate(offset);

    // Setup animators.
    weapon_animator_cb_[i] = [&, i]()->void {
      beam_[i].SetColor({1, 1, 1, 1});
      beam_[i].SetVisible(true);
      beam_spark_[i].SetVisible(true);
      spark_animator_[i].Play(eng::Animator::kMovement, false);
      weapon_[i].SetFrame(wepon_cooldown_frame[i]);
      weapon_animator_[i].UpdateStartValues(eng::Animator::kFrames);
      weapon_animator_[i].SetFrames(wepon_cooldown_frame_count, wepon_anim_speed);
      weapon_animator_[i].SetEndCallback(eng::Animator::kFrames, [&, i]()->void {
        weapon_[i].SetFrame(wepon_fire_frame[i]);
        weapon_animator_[i].UpdateStartValues(eng::Animator::kFrames);
      });
      weapon_animator_[i].Play(eng::Animator::kFrames, false);
    };
    weapon_animator_[i].SetRotation(M_PI * 2, 0.5f);
    weapon_animator_[i].Attach(&weapon_[i]);
    weapon_animator_[i].Play(eng::Animator::kRotation, true);

    spark_animator_[i].SetEndCallback(eng::Animator::kMovement, [&, i]()->void {
      beam_spark_[i].SetVisible(false);
      beam_animator_[i].Play(eng::Animator::kBlending, false);
      static_cast<Demo*>(engine.GetGame())->GetEnemy().HitTarget((DamageType)i);
    });
    spark_animator_[i].Attach(&beam_spark_[i]);

    beam_animator_[i].SetEndCallback(eng::Animator::kBlending, [&, i]()->void {
      beam_[i].SetVisible(false);
    });
    beam_animator_[i].SetBlending({1, 1, 1, 0}, 8);
    beam_animator_[i].Attach(&beam_[i]);
  }
}

void Player::DragStart(const Vector2& pos) {
  drag_start_ = pos;
  active_weapon_ = GetWeaponType(drag_start_);
  assert(active_weapon_ != kDamageType_Invalid);

  drag_sign_[active_weapon_].SetOffset(drag_start_);
  drag_sign_[active_weapon_].SetVisible(true);
}

void Player::Drag(const Vector2& pos) {
  assert(active_weapon_ != kDamageType_Invalid);

  drag_end_ = pos;
  drag_sign_[active_weapon_].SetOffset(drag_end_);
}

void Player::SelectTarget() {
  assert(active_weapon_ != kDamageType_Invalid);

  if (IsFiring(active_weapon_))
    return;

  eng::Engine& engine = eng::Engine::Get();
  Demo* game = static_cast<Demo*>(engine.GetGame());

  if (!ValidateDrag()) {
    game->GetEnemy().DeselectTarget((DamageType)active_weapon_);
    return;
  }

  game->GetEnemy().SelectTarget((DamageType)active_weapon_,
      weapon_[active_weapon_].GetOffset(), drag_end_);
}

void Player::DragEnd() {
  assert(active_weapon_ != kDamageType_Invalid);

  DamageType type = active_weapon_;
  active_weapon_ = kDamageType_Invalid;
  drag_sign_[type].SetVisible(false);

  eng::Engine& engine = eng::Engine::Get();
  Demo* game = static_cast<Demo*>(engine.GetGame());

  if (IsFiring(type))
    return;

  if (!ValidateDrag()) {
    game->GetEnemy().DeselectTarget((DamageType)type);
    return;
  }

  Fire(type);
  type = kDamageType_Invalid;
}

bool Player::ValidateDrag() {
  Vector2 dir = drag_end_ - drag_start_;
  float len = dir.Magnitude();
  dir.Normalize();
  if (len < weapon_[active_weapon_].GetScale().y / 6)
    return false;
  if (dir.DotProduct(Vector2(0 ,1)) < 0)
    return false;
  return true;
}
