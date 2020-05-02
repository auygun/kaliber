#include "player.h"
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
    beam_dot_animator_[i].Update(delta_time);
    beam_spark_animator_[i].Update(delta_time);
  }
}

void Player::OnInputEvent(std::unique_ptr<engine::InputEvent> event) {
  if (event->GetEventType() == engine::InputEvent::kDragStart) {
    drag_start_ = event->GetEventVector(0);
    SetActiveWeapon(drag_start_);
    if (active_weapon_ >=0 ) {
      drag_sign_[active_weapon_].SetOffset(drag_start_);
      drag_sign_[active_weapon_].SetVisible(true);
    }
  } else if (event->GetEventType() == engine::InputEvent::kDrag) {
    drag_end_ = event->GetEventVector(0);
    if (active_weapon_ >=0 )
      drag_sign_[active_weapon_].SetOffset(drag_end_);
  } else if (event->GetEventType() == engine::InputEvent::kDragEnd) {
    if (active_weapon_ >=0 ) {
      drag_sign_[active_weapon_].SetVisible(false);
      Fire(active_weapon_);
    }
  }
}

void Player::SetActiveWeapon(const Vector2& pos) {
  active_weapon_ = -1;
  for (int i = 0; i < 2; ++i) {
    if ((drag_start_ - weapon_[i].offset()).Magnitude() < weapon_[i].scale().x)
      active_weapon_ = i;
  }
}

void Player::SetBeamLength(int i, float len) {
  engine::Engine& engine = engine::Engine::Get();

  beam_[i].SetOffset({0, 0});
  beam_[i].ResetScale();
  beam_[i].Scale({len, 2});
  beam_[i].PlaceToRightOf(weapon_[i]);
  beam_[i].Translate(weapon_[i].scale() * Vector2(-0.5f, 0));
  beam_[i].SetPivot(beam_[i].offset());

  Vector2 offset = engine.GetScreenSize() / Vector2(i ? -4 : 4 , -2)
      + Vector2(0, weapon_[i].scale().y);
  beam_[i].Translate(offset);
}

void Player::Fire(int i) {
  if (IsFiring(i))
    return;

  Vector2 dir = drag_start_ - drag_end_;
  float len = dir.Magnitude();
  if (len < weapon_[i].scale().y)
    return;

  SetBeamLength(i, len);

  dir.Normalize();
  float cos_angle = dir.DotProduct(Vector2(1 ,0));
  float angle = acos(cos_angle) + M_PI;
  beam_[i].Rotate(angle);
  beam_dot_[i].Rotate(angle);
  beam_spark_[i].Rotate(angle);

  beam_dot_animator_[i].SetMovement(-dir, beam_[i].scale().x * 0.9f);
  beam_spark_animator_[i].SetMovement(-dir, beam_[i].scale().x * 0.9f);

  weapon_animator_[i].Play(false);
}

bool Player::IsFiring(int i) {
  return weapon_animator_[i].IsPlaying() ||
      beam_animator_[i].IsPlaying() ||
      beam_dot_animator_[i].IsPlaying() ||
      beam_spark_animator_[i].IsPlaying();
}

void Player::SetupWeapons() {
  engine::Engine& engine = engine::Engine::Get();

  auto weapon_image =
      engine.GetAssetManager().GetImage("enemy_anims_flare_ok.png");
  auto beam_image =
      engine.GetAssetManager().GetImage("enemy_ray_ok.png");
  auto beam_dot_image =
      engine.GetAssetManager().GetImage("enemy_ray_dot_ok.png");

  for (int i = 0; i < 2; ++i) {
    // Setup draw sign.
    drag_sign_[i].Create(weapon_image, {8, 2});
    drag_sign_[i].SetCurrentFrame(i * 8);
    drag_sign_[i].Scale(2);
    engine.AddDrawable(&drag_sign_[i]);

    // Setup beam.
    beam_[i].Create(beam_image, {1, 2});
    beam_[i].SetCurrentFrame(i);
    beam_[i].Scale(2);
    beam_[i].PlaceToRightOf(weapon_[i]);
    beam_[i].Translate(weapon_[i].scale() * Vector2(-0.5f, 0));
    beam_[i].SetPivot(beam_[i].offset());

    // Setup beam dot.
    beam_dot_[i].Create(beam_dot_image, {1, 2});
    beam_dot_[i].SetCurrentFrame(i);
    beam_dot_[i].Scale(2);
    beam_dot_[i].PlaceToRightOf(weapon_[i]);
    beam_dot_[i].Translate(weapon_[i].scale() * Vector2(-0.5f, 0));
    beam_dot_[i].SetPivot(beam_dot_[i].offset());

    // Setup beam spark.
    beam_spark_[i].Create(weapon_image, {8, 2});
    beam_spark_[i].SetCurrentFrame(i * 8 + 1);
    beam_spark_[i].Scale(2);
    beam_spark_[i].PlaceToRightOf(weapon_[i]);
    beam_spark_[i].Translate(weapon_[i].scale() * Vector2(-0.5f, 0));
    beam_spark_[i].SetPivot(beam_spark_[i].offset());

    // Setup weapon.
    weapon_[i].Create(weapon_image, {8, 2});
    weapon_[i].Scale(2);
    weapon_[i].SetVisible(true);

    // Place parts.
    Vector2 offset = engine.GetScreenSize() / Vector2(i ? -4 : 4 , -2)
        + Vector2(0, weapon_[i].scale().y);
    beam_[i].Translate(offset);
    beam_dot_[i].Translate(offset);
    beam_spark_[i].Translate(offset);
    weapon_[i].Translate(offset);

    // Add to drawables.
    engine.AddDrawable(&beam_[i]);
    engine.AddDrawable(&beam_dot_[i]);
    engine.AddDrawable(&beam_spark_[i]);
    engine.AddDrawable(&weapon_[i]);

    // Setup animators.
    weapon_animator_[i].AttachFrameController(&weapon_[i]);
    weapon_animator_[i].SetSpeed(0.016f);
    weapon_animator_[i].SetFrameRange(i * 8 + 1, i * 8 + 8);
    weapon_animator_[i].SetIdleFrame(i * 8 + 1);
    weapon_animator_[i].SetCallback(5, [&, i]()->void {
      beam_[i].SetColor({1, 1, 1, 1});
      beam_[i].SetVisible(true);
      beam_dot_[i].SetVisible(true);
      beam_dot_animator_[i].Play(false);
    });
    beam_dot_animator_[i].AttachDrawable(&beam_dot_[i]);
    beam_dot_animator_[i].SetSpeed(0.2f);
    beam_dot_animator_[i].SetCallback([&, i]()->void {
      beam_dot_animator_[i].Stop();
      beam_dot_[i].SetVisible(false);
      beam_spark_[i].SetVisible(true);
      beam_spark_animator_[i].Play(false);
    });
    beam_spark_animator_[i].AttachDrawable(&beam_spark_[i]);
    beam_spark_animator_[i].SetSpeed(0.2f);
    beam_spark_animator_[i].SetCallback([&, i]()->void {
      beam_spark_animator_[i].Stop();
      beam_spark_[i].SetVisible(false);
      beam_animator_[i].FadeOut();
    });
    beam_animator_[i].AttachDrawable(&beam_[i]);
    beam_animator_[i].SetSpeed(8);
  }
}
