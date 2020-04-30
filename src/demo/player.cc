#include "player.h"
#include "../base/log.h"
#include "../engine/asset_manager/image.h"
#include "../engine/engine.h"
#include "../engine/input_event.h"
#include <math.h>
#include <memory>

bool Player::Initialize() {
  if (!CreateBeam())
    return false;
  return true;
}

void Player::Update(float delta_time) {
  if (beam_start_.visible()) {
    float cos_angle = start_pos_.DotProduct(end_pos_);
    RotateBeam(acos(cos_angle) - M_PI_2);
  }
}

void Player::OnInputEvent(std::unique_ptr<engine::InputEvent> event) {
  if (event->GetEventType() == engine::InputEvent::kDragStart) {
    start_pos_ = event->GetEventVector(0).Normalize();
    SetBeamVisible(true);
  } else if (event->GetEventType() == engine::InputEvent::kDrag) {
    end_pos_ = event->GetEventVector(0).Normalize();
  } else if (event->GetEventType() == engine::InputEvent::kDragEnd) {
    SetBeamVisible(false);
  }
}

bool Player::CreateBeam() {
  engine::Engine& engine = engine::Engine::Get();

  std::shared_ptr<const engine::Image> image_start;
  std::shared_ptr<const engine::Image> image_mid;
  std::shared_ptr<const engine::Image> image_end;
  image_start = engine.GetAssetManager().GetImage("gbeam_start.png");
  image_mid = engine.GetAssetManager().GetImage("gbeam_mid.png");
  image_end = engine.GetAssetManager().GetImage("gbeam_end.png");
  if (!beam_start_.Create(image_start)) {
    LOG << "Failed to create the sprite.";
    return false;
  }
  if (!beam_mid_.Create(image_mid)) {
    LOG << "Failed to create the sprite.";
    return false;
  }
  if (!beam_end_.Create(image_end)) {
    LOG << "Failed to create the sprite.";
    return false;
  }

  beam_start_.PlaceToLeftOf(beam_mid_);
  beam_end_.PlaceToRightOf(beam_mid_);

  Vector2 center_offset =
      {beam_start_.scale().x / 2 +  beam_mid_.scale().x / 2 + beam_end_.scale().x / 2, 0};
  beam_start_.ResetCenter(center_offset);
  beam_mid_.ResetCenter(center_offset);
  beam_end_.ResetCenter(center_offset);

  TranslateBeam({0, -engine::Engine::Get().GetScreenSize().y / 2 + 0.2f});

  engine::Engine::Get().AddDrawable(&beam_start_);
  engine::Engine::Get().AddDrawable(&beam_mid_);
  engine::Engine::Get().AddDrawable(&beam_end_);

  return true;
}

void Player::TranslateBeam(const Vector2& offset) {
  beam_start_.Translate(offset);
  beam_mid_.Translate(offset);
  beam_end_.Translate(offset);
}

void Player::RotateBeam(float angle) {
  beam_start_.Rotate(angle);
  beam_mid_.Rotate(angle);
  beam_end_.Rotate(angle);
}

void Player::SetBeamVisible(bool visible) {
  beam_start_.SetVisible(visible);
  beam_mid_.SetVisible(visible);
  beam_end_.SetVisible(visible);
}
