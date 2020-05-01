#include "player.h"
#include "../base/log.h"
#include "../engine/asset_manager/image.h"
#include "../engine/engine.h"
#include "../engine/input_event.h"
#include <math.h>
#include <memory>

bool Player::Initialize() {
  CreateWeapon();
  return true;
}

void Player::Update(float delta_time) {
  for (int i = 0; i < 2; ++i)
    weapon_animator_[i].Update(delta_time);
}

void Player::OnInputEvent(std::unique_ptr<engine::InputEvent> event) {
  if (event->GetEventType() == engine::InputEvent::kDragStart) {
    drag_start_ = event->GetEventVector(0).Normalize();
  } else if (event->GetEventType() == engine::InputEvent::kDrag) {
    drag_end_ = event->GetEventVector(0).Normalize();
  } else if (event->GetEventType() == engine::InputEvent::kDragEnd) {
    Fire();
  }
}

void Player::Fire() {
  // if (beam_start_.visible()) {
  //   float cos_angle = drag_start_.DotProduct(drag_end_);
  //   RotateBeam(acos(cos_angle) - M_PI_2);
  // }
}

void Player::CreateWeapon() {
  engine::Engine& engine = engine::Engine::Get();

  auto weapon_image =
      engine.GetAssetManager().GetImage("enemy_anims_flare_ok.png");
  auto beam_image =
      engine.GetAssetManager().GetImage("enemy_ray_ok.png");
  for (int i = 0; i < 2; ++i) {
    weapon_[i].Create(weapon_image, {8, 2});
    beam_[i].Create(beam_image, {1, 2});

    beam_[i].SetCurrentFrame(i);
    beam_[i].PlaceToRightOf(weapon_[i]);
    beam_[i].Translate(beam_[i].offset() * Vector2(-0.2f, 0));

    beam_[i].ResetCenter({0, 0});
    weapon_[i].ResetCenter({0, 0});

    Vector2 offset = engine.GetScreenSize() / Vector2(i ? -4 : 4 , -2)
        + Vector2(0, weapon_[i].scale().y);
    beam_[i].Translate(offset);
    weapon_[i].Translate(offset);

    weapon_[i].SetVisible(true);
    beam_[i].SetVisible(true);

    engine.AddDrawable(&beam_[i]);
    engine.AddDrawable(&weapon_[i]);

    weapon_animator_[i].AttachFrameController(&weapon_[i]);
    weapon_animator_[i].SetFrameRange(i * 8 + 1, i * 8 + 8);
    weapon_animator_[i].SetIdleFrame(i * 8 + 1);
    weapon_animator_[i].Play(false);
  }
}
