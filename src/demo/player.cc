#include "player.h"

#include "../base/log.h"
#include "../engine/engine.h"
#include "../engine/image.h"
#include "../engine/input_event.h"
#include "demo.h"

using namespace base;
using namespace eng;

namespace {

constexpr int wepon_warmup_frame[] = {1, 9};
constexpr int wepon_warmup_frame_count = 4;
constexpr int wepon_cooldown_frame[] = {5, 13};
constexpr int wepon_cooldown_frame_count = 3;
constexpr int wepon_anim_speed = 48;

}  // namespace

Player::Player() = default;

Player::~Player() = default;

bool Player::Initialize() {
  if (!CreateRenderResources())
    return false;
  SetupWeapons();
  return true;
}

void Player::Update(float delta_time) {
  if (active_weapon_ != kDamageType_Invalid)
    UpdateTarget();
}

void Player::Pause(bool pause) {
  for (int i = 0; i < 2; ++i) {
    warmup_animator_[i].PauseOrResumeAll(pause);
    cooldown_animator_[i].PauseOrResumeAll(pause);
    beam_animator_[i].PauseOrResumeAll(pause);
    spark_animator_[i].PauseOrResumeAll(pause);
  }
}

void Player::OnInputEvent(std::unique_ptr<InputEvent> event) {
  if (event->GetType() == InputEvent::kNavigateBack)
    NavigateBack();
  else if (event->GetType() == InputEvent::kDragStart)
    DragStart(event->GetVector());
  else if (event->GetType() == InputEvent::kDrag)
    Drag(event->GetVector());
  else if (event->GetType() == InputEvent::kDragEnd)
    DragEnd();
  else if (event->GetType() == InputEvent::kDragCancel)
    DragCancel();
}

Vector2f Player::GetWeaponPos(DamageType type) const {
  return Engine::Get().GetScreenSize() /
             Vector2f(type == kDamageType_Green ? 3.5f : -3.5f, -2) +
         Vector2f(0, weapon_[type].GetSize().y * 0.7f);
}

Vector2f Player::GetWeaponScale() const {
  return weapon_[0].GetSize();
}

DamageType Player::GetWeaponType(const Vector2f& pos) {
  DamageType closest_weapon = kDamageType_Invalid;
  float closest_dist = std::numeric_limits<float>::max();
  for (int i = 0; i < 2; ++i) {
    float dist = (pos - weapon_[i].GetPosition()).Length();
    if (dist < closest_dist) {
      closest_dist = dist;
      closest_weapon = (DamageType)i;
    }
  }

  DCHECK(closest_weapon != kDamageType_Invalid);
  if (closest_dist < weapon_[closest_weapon].GetSize().x * 0.9f)
    return closest_weapon;
  return kDamageType_Invalid;
}

void Player::WarmupWeapon(DamageType type) {
  cooldown_animator_[type].Stop(Animator::kFrames);
  warmup_animator_[type].Play(Animator::kFrames, false);
}

void Player::CooldownWeapon(DamageType type) {
  warmup_animator_[type].Stop(Animator::kFrames);
  cooldown_animator_[type].Play(Animator::kFrames, false);
}

void Player::Fire(DamageType type, Vector2f dir) {
  Engine& engine = Engine::Get();
  Enemy& enemy = static_cast<Demo*>(engine.GetGame())->GetEnemy();

  if (enemy.HasTarget(type))
    dir = weapon_[type].GetPosition() - enemy.GetTargetPos(type);
  else
    dir *= engine.GetScreenSize().y * 1.3f;

  float len = dir.Length();
  beam_[type].SetSize({len, beam_[type].GetSize().y});
  beam_[type].SetPosition(weapon_[type].GetPosition());

  dir.Normalize();
  float cos_theta = dir.DotProduct(Vector2f(1, 0));
  float theta = acos(cos_theta) + M_PI;
  beam_[type].SetTheta(theta);
  auto offset = beam_[type].GetRotation() * (len / 2);
  beam_[type].Translate({offset.y, -offset.x});

  beam_[type].SetColor({1, 1, 1, 1});
  beam_[type].SetVisible(true);
  beam_spark_[type].SetVisible(true);

  spark_animator_[type].Stop(Animator::kMovement);
  float length = beam_[type].GetSize().x * 0.9f;
  Vector2f movement = dir * -length;
  // Convert from units per second to duration.
  float speed = 1.0f / (18.0f / length);
  spark_animator_[type].SetMovement(movement, speed);
  spark_animator_[type].Play(Animator::kMovement, false);
}

bool Player::IsFiring(DamageType type) {
  return beam_animator_[type].IsPlaying(Animator::kBlending) ||
         spark_animator_[type].IsPlaying(Animator::kMovement);
}

void Player::SetupWeapons() {
  for (int i = 0; i < 2; ++i) {
    // Setup draw sign.
    drag_sign_[i].Create("weapon_tex", {8, 2});
    drag_sign_[i].SetZOrder(21);
    drag_sign_[i].SetFrame(i * 8);

    Vector2f pos = GetWeaponPos((DamageType)i);

    // Setup weapon.
    weapon_[i].Create("weapon_tex", {8, 2});
    weapon_[i].SetZOrder(24);
    weapon_[i].SetVisible(true);
    weapon_[i].SetFrame(wepon_warmup_frame[i]);
    weapon_[i].SetPosition(pos);

    // Setup beam.
    beam_[i].Create("beam_tex", {1, 2});
    beam_[i].SetZOrder(22);
    beam_[i].SetFrame(i);
    beam_[i].SetPosition(pos);
    beam_[i].Translate(beam_[i].GetSize() * Vector2f(-0.5f, -0.5f));

    // Setup beam spark.
    beam_spark_[i].Create("weapon_tex", {8, 2});
    beam_spark_[i].SetZOrder(23);
    beam_spark_[i].SetFrame(i * 8 + 1);
    beam_spark_[i].SetPosition(pos);

    // Setup animators.
    weapon_[i].SetFrame(wepon_cooldown_frame[i]);
    cooldown_animator_[i].SetFrames(wepon_cooldown_frame_count,
                                    wepon_anim_speed);
    cooldown_animator_[i].SetEndCallback(Animator::kFrames, [&, i]() -> void {
      weapon_[i].SetFrame(wepon_warmup_frame[i]);
    });
    cooldown_animator_[i].Attach(&weapon_[i]);

    weapon_[i].SetFrame(wepon_warmup_frame[i]);
    warmup_animator_[i].SetFrames(wepon_warmup_frame_count, wepon_anim_speed);
    warmup_animator_[i].SetRotation(M_PI * 2, 8.0f);
    warmup_animator_[i].Attach(&weapon_[i]);
    warmup_animator_[i].Play(Animator::kRotation, true);

    spark_animator_[i].SetEndCallback(Animator::kMovement, [&, i]() -> void {
      beam_spark_[i].SetVisible(false);
      beam_animator_[i].Play(Animator::kBlending, false);
      static_cast<Demo*>(Engine::Get().GetGame())
          ->GetEnemy()
          .HitTarget((DamageType)i);
    });
    spark_animator_[i].Attach(&beam_spark_[i]);

    beam_animator_[i].SetEndCallback(
        Animator::kBlending, [&, i]() -> void { beam_[i].SetVisible(false); });
    beam_animator_[i].SetBlending({1, 1, 1, 0}, 0.16f);
    beam_animator_[i].Attach(&beam_[i]);
  }
}

void Player::UpdateTarget() {
  if (IsFiring(active_weapon_))
    return;

  Engine& engine = Engine::Get();
  Demo* game = static_cast<Demo*>(engine.GetGame());

  if (drag_valid_) {
    Vector2f dir = (drag_end_ - drag_start_).Normalize();
    game->GetEnemy().SelectTarget(active_weapon_, drag_start_, dir, 1.2f);
    if (!game->GetEnemy().HasTarget(active_weapon_))
      game->GetEnemy().SelectTarget(active_weapon_, drag_start_, dir, 2);
  } else {
    game->GetEnemy().DeselectTarget(active_weapon_);
  }
}

void Player::DragStart(const Vector2f& pos) {
  active_weapon_ = GetWeaponType(pos);
  if (active_weapon_ == kDamageType_Invalid)
    return;

  drag_start_ = pos;
  drag_end_ = pos;

  drag_sign_[active_weapon_].SetPosition(drag_start_);
  drag_sign_[active_weapon_].SetVisible(true);
}

void Player::Drag(const Vector2f& pos) {
  if (active_weapon_ == kDamageType_Invalid)
    return;

  drag_end_ = pos;
  drag_sign_[active_weapon_].SetPosition(drag_end_);

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

  UpdateTarget();

  DamageType type = active_weapon_;
  active_weapon_ = kDamageType_Invalid;
  drag_sign_[type].SetVisible(false);

  Vector2f fire_dir = (drag_start_ - drag_end_).Normalize();

  if (drag_valid_ && !IsFiring(type)) {
    if (warmup_animator_[type].IsPlaying(Animator::kFrames)) {
      warmup_animator_[type].SetEndCallback(
          Animator::kFrames, [&, type, fire_dir]() -> void {
            warmup_animator_[type].SetEndCallback(Animator::kFrames, nullptr);
            CooldownWeapon(type);
            Fire(type, fire_dir);
          });
    } else {
      CooldownWeapon(type);
      Fire(type, fire_dir);
    }
  }

  drag_valid_ = false;
  drag_start_ = {0, 0};
  drag_end_ = {0, 0};
}

void Player::DragCancel() {
  if (active_weapon_ == kDamageType_Invalid)
    return;

  DamageType type = active_weapon_;
  active_weapon_ = kDamageType_Invalid;
  drag_sign_[type].SetVisible(false);

  if (drag_valid_ && !IsFiring(type)) {
    if (warmup_animator_[type].IsPlaying(Animator::kFrames)) {
      warmup_animator_[type].SetEndCallback(
          Animator::kFrames, [&, type]() -> void {
            warmup_animator_[type].SetEndCallback(Animator::kFrames, nullptr);
            CooldownWeapon(type);
          });
    } else {
      CooldownWeapon(type);
    }
  }

  drag_valid_ = false;
  drag_start_ = {0, 0};
  drag_end_ = {0, 0};
}

bool Player::ValidateDrag() {
  Vector2f dir = drag_end_ - drag_start_;
  float len = dir.Length();
  dir.Normalize();
  if (len < weapon_[active_weapon_].GetSize().y / 4)
    return false;
  if (dir.DotProduct(Vector2f(0, 1)) < 0)
    return false;
  return true;
}

void Player::NavigateBack() {
  DragCancel();
  Engine& engine = Engine::Get();
  static_cast<Demo*>(engine.GetGame())->EnterMenuState();
}

bool Player::CreateRenderResources() {
  Engine::Get().SetImageSource("weapon_tex", "enemy_anims_flare_ok.png", true);
  Engine::Get().SetImageSource("beam_tex", "enemy_ray_ok.png", true);

  return true;
}
