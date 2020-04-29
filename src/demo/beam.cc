#include "beam.h"
#include "../base/log.h"
#include "../engine/asset_manager/image.h"
#include "../engine/engine.h"
#include "../engine/input_event.h"
#include <math.h>
#include <memory>

bool Beam::Initialize() {
  engine::Engine& engine = engine::Engine::Get();

  std::shared_ptr<const engine::Image> image_start;
  std::shared_ptr<const engine::Image> image_mid;
  std::shared_ptr<const engine::Image> image_end;
  image_start = engine.GetAssetManager().GetImage("gbeam_start.png");
  image_mid = engine.GetAssetManager().GetImage("gbeam_mid.png");
  image_end = engine.GetAssetManager().GetImage("gbeam_end.png");
  if (!start_.Create(image_start)) {
    LOG << "Failed to create the sprite.";
    return false;
  }
  if (!mid_.Create(image_mid)) {
    LOG << "Failed to create the sprite.";
    return false;
  }
  if (!end_.Create(image_end)) {
    LOG << "Failed to create the sprite.";
    return false;
  }

  start_.PlaceToLeftOf(mid_);
  end_.PlaceToRightOf(mid_);

  Vector2 center_offset =
      {start_.scale().x / 2 +  mid_.scale().x / 2 + end_.scale().x / 2, 0};
  start_.ResetCenter(center_offset);
  mid_.ResetCenter(center_offset);
  end_.ResetCenter(center_offset);

  Translate({0, -engine::Engine::Get().GetScreenSize().y / 2 + 0.2f});
  Rotate(-M_PI_2);

  engine::Engine::Get().AddDrawable(&start_);
  engine::Engine::Get().AddDrawable(&mid_);
  engine::Engine::Get().AddDrawable(&end_);

  return true;
}

void Beam::Update(float delta_time) {
  if (start_.visible()) {
    float cos_angle = start_pos_.DotProduct(end_pos_);
    Rotate(acos(cos_angle) - M_PI_2);
  }
}

void Beam::OnInputEvent(std::unique_ptr<engine::InputEvent> event) {
  if (event->GetEventType() == engine::InputEvent::kDragStart) {
    start_pos_ = event->GetEventVector(0).Normalize();
    SetVisible(true);
  } else if (event->GetEventType() == engine::InputEvent::kDrag) {
    end_pos_ = event->GetEventVector(0).Normalize();
  } else if (event->GetEventType() == engine::InputEvent::kDragEnd) {
    SetVisible(false);
  }
}

void Beam::Rotate(float angle) {
  start_.Rotate(angle);
  mid_.Rotate(angle);
  end_.Rotate(angle);
}

void Beam::Translate(const Vector2& offset) {
  start_.Translate(offset);
  mid_.Translate(offset);
  end_.Translate(offset);
}

void Beam::SetVisible(bool visible) {
  start_.SetVisible(visible);
  mid_.SetVisible(visible);
  end_.SetVisible(visible);
}
