#include "player.h"

#include "../base/interpolation.h"
#include "../base/log.h"
#include "../engine/engine.h"
#include "../engine/font.h"
#include "../engine/input_event.h"
#include "../engine/sound.h"
#include "demo.h"

using namespace base;
using namespace eng;

namespace {

constexpr int wepon_warmup_frame[] = {1, 9};
constexpr int wepon_warmup_frame_count = 4;
constexpr int wepon_cooldown_frame[] = {5, 13};
constexpr int wepon_cooldown_frame_count = 3;
constexpr int wepon_anim_speed = 48;

const Vector4 kNukeColor[2] = {{0.16f, 0.46f, 0.93f, 0},
                               {0.93f, 0.35f, 0.15f, 1}};

}  // namespace

Player::Player() = default;

Player::~Player() = default;

bool Player::Initialize() {
  if (!CreateRenderResources())
    return false;

  laser_shot_sound_ = std::make_shared<Sound>();
  if (!laser_shot_sound_->Load("laser.mp3", false))
    return false;

  nuke_explosion_sound_ = std::make_shared<Sound>();
  if (!nuke_explosion_sound_->Load("nuke.mp3", false))
    return false;

  SetupWeapons();

  Vector2 hb_pos = Engine::Get().GetScreenSize() / Vector2(2, -2) +
                   Vector2(0, weapon_[0].GetScale().y * 0.3f);

  for (int i = 0; i < 3; ++i) {
    health_bead_[i].Create("health_bead", {1, 2});
    health_bead_[i].SetZOrder(25);
    health_bead_[i].Translate(hb_pos * Vector2(0, 1));
    health_bead_[i].SetVisible(true);
  }
  health_bead_[0].PlaceToLeftOf(health_bead_[1]);
  health_bead_[0].Translate(health_bead_[1].GetScale() * Vector2(-0.1f, 0));
  health_bead_[2].PlaceToRightOf(health_bead_[1]);
  health_bead_[2].Translate(health_bead_[1].GetScale() * Vector2(0.1f, 0));

  nuke_symbol_.Create("nuke_symbol_tex", {5, 1});
  nuke_symbol_.SetZOrder(29);
  nuke_symbol_.SetOffset({0, weapon_[0].GetOffset().y});
  nuke_symbol_.SetFrame(4);
  nuke_symbol_.SetVisible(true);

  nuke_.SetZOrder(20);
  nuke_.Scale(Engine::Get().GetScreenSize());
  nuke_.SetColor(kNukeColor[0]);

  nuke_animator_.Attach(&nuke_);

  nuke_symbol_animator_.Attach(&nuke_symbol_);

  nuke_explosion_.SetSound(nuke_explosion_sound_);
  nuke_explosion_.SetVariate(false);
  nuke_explosion_.SetSimulateStereo(false);
  nuke_explosion_.SetMaxAplitude(0.8f);

  return true;
}

void Player::Update(float delta_time) {
  for (int i = 0; i < 2; ++i) {
    warmup_animator_[i].Update(delta_time);
    cooldown_animator_[i].Update(delta_time);
    beam_animator_[i].Update(delta_time);
    spark_animator_[i].Update(delta_time);
  }
  nuke_animator_.Update(delta_time);
  nuke_symbol_animator_.Update(delta_time);

  for (int i = 0; i < 2; ++i) {
    if (drag_weapon_[i] != kDamageType_Invalid)
      UpdateTarget(drag_weapon_[i]);
  }
}

void Player::OnInputEvent(std::unique_ptr<InputEvent> event) {
  if (event->GetType() == InputEvent::kNavigateBack)
    NavigateBack();
  else if (event->GetType() == InputEvent::kDragStart)
    DragStart(event->GetPointerId(), event->GetVector());
  else if (event->GetType() == InputEvent::kDrag)
    Drag(event->GetPointerId(), event->GetVector());
  else if (event->GetType() == InputEvent::kDragEnd)
    DragEnd(event->GetPointerId());
  else if (event->GetType() == InputEvent::kDragCancel)
    DragCancel(event->GetPointerId());
}

void Player::TakeDamage(int damage) {
  if (damage > 0)
    Engine::Get().Vibrate(250);

  hit_points_ = std::min(total_health_, std::max(0, hit_points_ - damage));

  for (int i = 0; i < 3; ++i)
    health_bead_[i].SetFrame(hit_points_ > i ? 0 : 1);

  if (hit_points_ == 0)
    static_cast<Demo*>(Engine::Get().GetGame())->EnterGameOverState();
}

void Player::AddNuke(int n) {
  int new_nuke_count = std::max(std::min(nuke_count_ + n, 3), 0);
  if (new_nuke_count == nuke_count_)
    return;

  nuke_count_ = new_nuke_count;
  nuke_symbol_.SetFrame(4 - nuke_count_);

  if (!nuke_symbol_animator_.IsPlaying(Animator::kRotation)) {
    nuke_symbol_animator_.SetRotation(
        M_PI * 5, 2, std::bind(SmootherStep, std::placeholders::_1));
    nuke_symbol_animator_.Play(Animator::kRotation, false);
  }
}

void Player::Reset() {
  DragCancel(0);
  DragCancel(1);

  TakeDamage(-total_health_);

  nuke_count_ = 1;
  nuke_symbol_.SetFrame(3);
}

Vector2 Player::GetWeaponPos(DamageType type) const {
  return Engine::Get().GetScreenSize() /
             Vector2(type == kDamageType_Green ? 3.5f : -3.5f, -2) +
         Vector2(0, weapon_[type].GetScale().y * 0.7f);
}

Vector2 Player::GetWeaponScale() const {
  return weapon_[0].GetScale();
}

DamageType Player::GetWeaponType(const Vector2& pos) {
  DamageType closest_weapon = kDamageType_Invalid;
  float closest_dist = std::numeric_limits<float>::max();
  for (int i = 0; i < 2; ++i) {
    float dist = (pos - weapon_[i].GetOffset()).Magnitude();
    if (dist < closest_dist) {
      closest_dist = dist;
      closest_weapon = (DamageType)i;
    }
  }

  DCHECK(closest_weapon != kDamageType_Invalid);
  if (closest_dist < weapon_[closest_weapon].GetScale().x * 0.5f)
    return closest_weapon;
  return kDamageType_Invalid;
}

void Player::SetBeamLength(DamageType type, float len) {
  beam_[type].SetOffset({0, 0});
  beam_[type].SetScale({len, beam_[type].GetScale().y});
  beam_[type].PlaceToRightOf(weapon_[type]);
  beam_[type].Translate(weapon_[type].GetScale() * Vector2(-0.5f, 0));
  beam_[type].SetPivot(beam_[type].GetOffset());
  beam_[type].Translate(weapon_[type].GetOffset());
}

void Player::WarmupWeapon(DamageType type) {
  cooldown_animator_[type].Stop(Animator::kFrames);
  warmup_animator_[type].Play(Animator::kFrames, false);
}

void Player::CooldownWeapon(DamageType type) {
  warmup_animator_[type].Stop(Animator::kFrames);
  cooldown_animator_[type].Play(Animator::kFrames, false);
}

void Player::Fire(DamageType type, Vector2 dir) {
  Engine& engine = Engine::Get();
  Enemy& enemy = static_cast<Demo*>(engine.GetGame())->GetEnemy();

  float max_beam_length = engine.GetScreenSize().y * 1.3f * 0.85f;
  constexpr float max_beam_duration = 0.259198f;

  if (enemy.HasTarget(type))
    dir = weapon_[type].GetOffset() - enemy.GetTargetPos(type);
  else
    dir *= engine.GetScreenSize().y * 1.3f;

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

  spark_animator_[type].Stop(Animator::kMovement);

  float length = beam_[type].GetScale().x * 0.85f;
  Vector2 movement = dir * -length;
  float duration = (length * max_beam_duration) / max_beam_length;

  spark_animator_[type].SetMovement(movement, duration);
  spark_animator_[type].Play(Animator::kMovement, false);

  laser_shot_[type].Play(false);
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

    // Setup weapon.
    weapon_[i].Create("weapon_tex", {8, 2});
    weapon_[i].SetZOrder(24);
    weapon_[i].SetVisible(true);
    weapon_[i].SetFrame(wepon_warmup_frame[i]);

    // Setup beam.
    beam_[i].Create("beam_tex", {1, 2});
    beam_[i].SetZOrder(22);
    beam_[i].SetFrame(i);
    beam_[i].PlaceToRightOf(weapon_[i]);
    beam_[i].Translate(weapon_[i].GetScale() * Vector2(-0.5f, 0));
    beam_[i].SetPivot(beam_[i].GetOffset());

    // Setup beam spark.
    beam_spark_[i].Create("weapon_tex", {8, 2});
    beam_spark_[i].SetZOrder(23);
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

    laser_shot_[i].SetSound(laser_shot_sound_);
    laser_shot_[i].SetVariate(true);
    laser_shot_[i].SetSimulateStereo(false);
    laser_shot_[i].SetMaxAplitude(0.4f);
  }
}

void Player::UpdateTarget(DamageType weapon) {
  if (IsFiring(weapon))
    return;

  Engine& engine = Engine::Get();
  Demo* game = static_cast<Demo*>(engine.GetGame());

  int i = weapon_drag_ind[weapon];

  if (drag_valid_[i]) {
    Vector2 origin = weapon_[weapon].GetOffset();
    Vector2 dir = (drag_end_[i] - drag_start_[i]).Normalize();
    game->GetEnemy().SelectTarget(weapon, origin, dir);
  } else {
    game->GetEnemy().DeselectTarget(weapon);
  }
}

void Player::Nuke() {
  if (nuke_count_ <= 0 || nuke_animator_.IsPlaying(Animator::kBlending))
    return;

  Engine& engine = Engine::Get();
  Demo* game = static_cast<Demo*>(engine.GetGame());

  AddNuke(-1);

  nuke_animator_.SetEndCallback(Animator::kBlending, [&, game]() -> void {
    nuke_animator_.SetEndCallback(Animator::kBlending,
                                  [&]() -> void { nuke_.SetVisible(false); });
    nuke_animator_.SetBlending(
        kNukeColor[0], 2, std::bind(Acceleration, std::placeholders::_1, -1));
    nuke_animator_.SetEndCallback(Animator::kTimer, [&, game]() -> void {
      game->GetEnemy().KillAllEnemyUnits(false);
      game->GetEnemy().ResumeProgress();
    });
    nuke_animator_.SetTimer(0.5f);
    nuke_animator_.Play(Animator::kBlending | Animator::kTimer, false);
  });
  nuke_animator_.SetBlending(kNukeColor[1], 0.1f,
                             std::bind(Acceleration, std::placeholders::_1, 1));
  nuke_animator_.Play(Animator::kBlending, false);
  nuke_.SetVisible(true);

  game->GetEnemy().PauseProgress();
  game->GetEnemy().StopAllEnemyUnits();

  nuke_explosion_.Play(false);
}

void Player::DragStart(int i, const Vector2& pos) {
  drag_weapon_[i] = GetWeaponType(pos);
  if (drag_weapon_[i] == kDamageType_Invalid) {
    float dist = (pos - nuke_symbol_.GetOffset()).Magnitude();
    drag_nuke_[i] = dist <= nuke_symbol_.GetScale().x * 0.7f;
    return;
  }

  weapon_drag_ind[drag_weapon_[i]] = i;
  drag_start_[i] = drag_end_[i] = pos;

  drag_sign_[drag_weapon_[i]].SetOffset(pos);
  drag_sign_[drag_weapon_[i]].SetVisible(true);
}

void Player::Drag(int i, const Vector2& pos) {
  if (drag_weapon_[i] == kDamageType_Invalid)
    return;

  drag_end_[i] = pos;
  drag_sign_[drag_weapon_[i]].SetOffset(pos);

  if (ValidateDrag(i)) {
    if (!drag_valid_[i] && !IsFiring(drag_weapon_[i]))
      WarmupWeapon(drag_weapon_[i]);
    drag_valid_[i] = true;
  } else {
    if (drag_valid_[i] && !IsFiring(drag_weapon_[i]))
      CooldownWeapon(drag_weapon_[i]);
    drag_valid_[i] = false;
  }
}

void Player::DragEnd(int i) {
  if (drag_weapon_[i] == kDamageType_Invalid) {
    if (drag_nuke_[i]) {
      drag_nuke_[i] = false;
      Nuke();
    }
    return;
  }

  UpdateTarget(drag_weapon_[i]);

  DamageType type = drag_weapon_[i];
  drag_weapon_[i] = kDamageType_Invalid;
  drag_sign_[type].SetVisible(false);

  Vector2 fire_dir = (drag_start_[i] - drag_end_[i]).Normalize();

  if (drag_valid_[i] && !IsFiring(type)) {
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

  drag_valid_[i] = false;
  drag_start_[i] = drag_end_[i] = {0, 0};
}

void Player::DragCancel(int i) {
  if (drag_weapon_[i] == kDamageType_Invalid)
    return;

  Engine& engine = Engine::Get();
  Demo* game = static_cast<Demo*>(engine.GetGame());

  game->GetEnemy().DeselectTarget(drag_weapon_[i]);

  DamageType type = drag_weapon_[i];
  drag_weapon_[i] = kDamageType_Invalid;
  drag_sign_[type].SetVisible(false);

  if (drag_valid_[i] && !IsFiring(type)) {
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

  drag_valid_[i] = false;
  drag_start_[i] = drag_end_[i] = {0, 0};
}

bool Player::ValidateDrag(int i) {
  Vector2 dir = drag_end_[i] - drag_start_[i];
  float len = dir.Magnitude();
  dir.Normalize();
  if (len < weapon_[0].GetScale().y / 3)
    return false;
  if (dir.DotProduct(Vector2(0, 1)) < 0)
    return false;
  return true;
}

void Player::NavigateBack() {
  DragCancel(0);
  DragCancel(1);
  Engine& engine = Engine::Get();
  static_cast<Demo*>(engine.GetGame())->EnterMenuState();
}

bool Player::CreateRenderResources() {
  Engine::Get().SetImageSource("weapon_tex", "enemy_anims_flare_ok.png", true);
  Engine::Get().SetImageSource("beam_tex", "enemy_ray_ok.png", true);
  Engine::Get().SetImageSource("nuke_symbol_tex", "nuke_frames.png", true);
  Engine::Get().SetImageSource("health_bead", "bead.png", true);

  return true;
}
