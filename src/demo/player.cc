#include "player.h"
#include "demo.h"
#include "../base/log.h"
#include "../engine/asset_manager/image.h"
#include "../engine/engine.h"
#include "../engine/input_event.h"
#include <math.h>
#include <memory>

bool Player::Initialize() {
  SetupWeapons();
  return true;
}

void Player::Update(float delta_time) {
  for (int i = 0; i < 2; ++i) {
    weapon_animator_[i].Update(delta_time);
    beam_animator_[i].Update(delta_time);
    beam_spark_animator_[i].Update(delta_time);
  }
}

void Player::OnInputEvent(std::unique_ptr<engine::InputEvent> event) {
  engine::Engine& engine = engine::Engine::Get();

  if (event->GetEventType() == engine::InputEvent::kDragStart) {
    drag_start_ = event->GetEventVector(0);
    active_weapon_ = GetWeaponType(drag_start_);
    if (active_weapon_ != kDamageType_Invalid) {
      // active_weapon_ = type;
      drag_sign_[active_weapon_].SetOffset(drag_start_);
      drag_sign_[active_weapon_].SetVisible(true);
    }
  } else if (event->GetEventType() == engine::InputEvent::kDrag) {
    drag_end_ = event->GetEventVector(0);
    if (active_weapon_ != kDamageType_Invalid) {
      drag_sign_[active_weapon_].SetOffset(drag_end_);
      if (!IsFiring(active_weapon_)) {
        Demo* game = static_cast<Demo*>(engine.GetGame());
        game->GetEnemy().SelectTarget((DamageType)active_weapon_,
            weapon_[active_weapon_].offset(), drag_end_);
      }
    }
  } else if (event->GetEventType() == engine::InputEvent::kDragEnd) {
    if (active_weapon_ != kDamageType_Invalid) {
      drag_sign_[active_weapon_].SetVisible(false);
      if (!IsFiring(active_weapon_))
        Fire(active_weapon_);
      active_weapon_ = kDamageType_Invalid;
    }
  }
}

Vector2 Player::GetWeaponPos(DamageType type) const {
  return weapon_[type].offset();
}

Vector2 Player::GetWeaponScale(DamageType type) const {
  return weapon_[type].scale();
}

DamageType Player::GetWeaponType(const Vector2& pos) {
  for (int i = 0; i < 2; ++i) {
    if ((drag_start_ - weapon_[i].offset()).Magnitude() < weapon_[i].scale().x)
      return (DamageType)i;
  }
  return kDamageType_Invalid;
}

void Player::SetBeamLength(DamageType type, float len) {
  engine::Engine& engine = engine::Engine::Get();

  beam_[type].SetOffset({0, 0});
  beam_[type].ResetScale();
  beam_[type].Scale({len, 2});
  beam_[type].PlaceToRightOf(weapon_[type]);
  beam_[type].Translate(weapon_[type].scale() * Vector2(-0.5f, 0));
  beam_[type].SetPivot(beam_[type].offset());

  Vector2 offset = engine.GetScreenSize() / Vector2(type ? -4 : 4 , -2)
      + Vector2(0, weapon_[type].scale().y);
  beam_[type].Translate(offset);
}

void Player::Fire(DamageType type) {
  engine::Engine& engine = engine::Engine::Get();
  Enemy &enemy = static_cast<Demo*>(engine.GetGame())->GetEnemy();

  Vector2 dir = weapon_[type].offset() -
      (enemy.HasTarget(type) ? enemy.GetTargetPos(type) : drag_end_);
  float len = dir.Magnitude();
  if (len < weapon_[type].scale().y)
    return;
  if (dir.DotProduct(Vector2(0 ,1)) >= 0)
    return;

  SetBeamLength(type, len);

  dir.Normalize();
  float cos_angle = dir.DotProduct(Vector2(1 ,0));
  float angle = acos(cos_angle) + M_PI;
  beam_[type].Rotate(angle);
  beam_spark_[type].Rotate(angle);

  beam_spark_animator_[type].SetMovement(-dir, beam_[type].scale().x * 0.9f);
  weapon_animator_[type].Play(false, true);
}

bool Player::IsFiring(DamageType type) {
  return weapon_animator_[type].IsPlaying() ||
      beam_animator_[type].IsPlaying() ||
      beam_spark_animator_[type].IsPlaying();
}

void Player::SetupWeapons() {
  engine::Engine& engine = engine::Engine::Get();

  auto weapon_image =
      engine.GetAssetManager().GetImage("enemy_anims_flare_ok.png");
  auto beam_image =
      engine.GetAssetManager().GetImage("enemy_ray_ok.png");

  for (int i = 0; i < 2; ++i) {
    // Setup draw sign.
    drag_sign_[i].Create(weapon_image, {8, 2});
    drag_sign_[i].SetCurrentFrame(i * 8);
    drag_sign_[i].Scale(2);
    engine.AddDrawable(&drag_sign_[i]);

    // Setup weapon.
    weapon_[i].Create(weapon_image, {8, 2});
    weapon_[i].Scale(2);
    weapon_[i].SetVisible(true);
    engine.AddDrawable(&weapon_[i]);

    // Setup beam.
    beam_[i].Create(beam_image, {1, 2});
    beam_[i].SetCurrentFrame(i);
    beam_[i].Scale(2);
    beam_[i].PlaceToRightOf(weapon_[i]);
    beam_[i].Translate(weapon_[i].scale() * Vector2(-0.5f, 0));
    beam_[i].SetPivot(beam_[i].offset());
    engine.AddDrawable(&beam_[i]);

    // Setup beam spark.
    beam_spark_[i].Create(weapon_image, {8, 2});
    beam_spark_[i].SetCurrentFrame(i * 8 + 1);
    beam_spark_[i].Scale(2);
    beam_spark_[i].PlaceToRightOf(weapon_[i]);
    beam_spark_[i].Translate(weapon_[i].scale() * Vector2(-0.5f, 0));
    beam_spark_[i].SetPivot(beam_spark_[i].offset());
    engine.AddDrawable(&beam_spark_[i]);

    // Place parts.
    Vector2 offset = engine.GetScreenSize() / Vector2(i ? -4 : 4 , -2)
        + Vector2(0, weapon_[i].scale().y);
    beam_[i].Translate(offset);
    beam_spark_[i].Translate(offset);
    weapon_[i].Translate(offset);

    // Setup animators.
    weapon_animator_[i].AttachFrameController(&weapon_[i]);
    weapon_animator_[i].SetSpeed(0.016f);
    weapon_animator_[i].SetFrameRange(i * 8 + 1, i * 8 + 8, i * 8 + 1);
    weapon_animator_[i].SetCallback(5, [&, i]()->void {
      beam_[i].SetColor({1, 1, 1, 1});
      beam_[i].SetVisible(true);
      beam_spark_[i].SetVisible(true);
      beam_spark_animator_[i].Play(false);
    });
    beam_spark_animator_[i].AttachDrawable(&beam_spark_[i]);
    beam_spark_animator_[i].SetSpeed(0.2f);
    beam_spark_animator_[i].SetCallback([&, i]()->void {
      beam_spark_animator_[i].Stop();
      beam_spark_[i].SetVisible(false);
      beam_animator_[i].FadeOut();
      static_cast<Demo*>(engine.GetGame())->GetEnemy().KillTarget((DamageType)i);
    });
    beam_animator_[i].AttachDrawable(&beam_[i]);
    beam_animator_[i].SetSpeed(8);
  }
}
