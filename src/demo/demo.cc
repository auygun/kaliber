#include "demo/demo.h"

#include <algorithm>
#include <atomic>
#include <iostream>
#include <string>

#include "base/file.h"
#include "base/interpolation.h"
#include "base/log.h"
#include "base/random.h"
#include "base/timer.h"
#include "engine/engine.h"
#include "engine/game_factory.h"
#include "engine/input_event.h"
#include "engine/sound.h"

DECLARE_GAME_BEGIN
DECLARE_GAME(Demo)
DECLARE_GAME_END

// #define RECORD 15
// #define REPLAY

using namespace std::string_literals;

using namespace base;
using namespace eng;

namespace {

const Vector4f kBgColor = {0, 0, 0, 0.8f};
constexpr float kFadeSpeed = 0.2f;

constexpr int kLaunchCountBeforeAd = 2;

const char kSaveFileName[] = "woom";
const char kHightScore[] = "high_score";
const char kLastWave[] = "last_wave";
const char kLaunchCount[] = "launch_count";

}  // namespace

Demo::Demo() = default;

Demo::~Demo() {
  saved_data_.Save();
}

bool Demo::Initialize() {
  saved_data_.Load(kSaveFileName);

  if (!font_.Load("PixelCaps!.ttf"))
    return false;

  if (!sky_.Create(false)) {
    LOG << "Could not create the sky.";
    return false;
  }

  if (!enemy_.Initialize()) {
    LOG << "Failed to create the enemy.";
    return false;
  }

  if (!player_.Initialize()) {
    LOG << "Failed to create the enemy.";
    return false;
  }

  if (!hud_.Initialize()) {
    LOG << "Failed to create the hud.";
    return false;
  }

  if (!menu_.Initialize()) {
    LOG << "Failed to create the menu.";
    return false;
  }

  if (!credits_.Initialize()) {
    LOG << "Failed to create the credits.";
    return false;
  }

  auto sound = std::make_unique<Sound>();
  if (!sound->Load("Game_2_Main.mp3", true))
    return false;

  auto boss_sound = std::make_unique<Sound>();
  if (!boss_sound->Load("Game_2_Boss.mp3", true))
    return false;

  music_.SetSound(std::move(sound));
  music_.SetMaxAplitude(0.5f);

  boss_music_.SetSound(std::move(boss_sound));
  boss_music_.SetMaxAplitude(0.5f);

  if (!saved_data_.root().get("audio", Json::Value(true)).asBool())
    Engine::Get().SetEnableAudio(false);
  else if (saved_data_.root().get("music", Json::Value(true)).asBool())
    music_.Play(true);

  if (!saved_data_.root().get("vibration", Json::Value(true)).asBool()) {
    Engine::Get().SetEnableVibration(false);
  }

  dimmer_.SetSize(Engine::Get().GetScreenSize());
  dimmer_.SetZOrder(40);
  dimmer_.SetColor(kBgColor);
  dimmer_.SetVisible(true);
  dimmer_active_ = true;
  dimmer_animator_.Attach(&dimmer_);

  saved_data_.root()[kLaunchCount] =
      saved_data_.root().get(kLaunchCount, Json::Value(0)).asInt() + 1;

  EnterMenuState();

  return true;
}

void Demo::Update(float delta_time) {
  Engine& engine = Engine::Get();

  if (do_benchmark_) {
    benchmark_time_ += delta_time;
    if (benchmark_time_ > 1) {
      avarage_fps_ += Engine::Get().fps();
      ++num_benchmark_samples_;
    }
    if (benchmark_time_ > 6) {
      avarage_fps_ /= num_benchmark_samples_;
      do_benchmark_ = false;
      BenchmarkResult(avarage_fps_);
    }
  }

  stage_time_ += delta_time;

  while (std::unique_ptr<InputEvent> event = engine.GetNextInputEvent()) {
#if 0
    if (event->GetType() == InputEvent::kDragEnd &&
        ((engine.GetScreenSize() / 2) * 0.9f -
         event->GetVector() * Vector2f(-1, 1))
                .Length() <= 0.25f)
      Win();
#endif

    if (state_ == kMenu)
      menu_.OnInputEvent(std::move(event));
    else if (state_ == kCredits)
      credits_.OnInputEvent(std::move(event));
    else if (state_ != kGameOver)
      player_.OnInputEvent(std::move(event));
  }

  if (state_ == kMenu)
    UpdateMenuState(delta_time);
  else if (state_ == kGame || state_ == kGameOver)
    UpdateGameState(delta_time);
}

void Demo::ContextLost() {
  sky_.ContextLost();
  enemy_.ContextLost();
}

void Demo::LostFocus() {}

void Demo::GainedFocus(bool from_interstitial_ad) {
  DLOG << __func__ << " from_interstitial_ad: " << from_interstitial_ad;
  if (!from_interstitial_ad) {
    if (saved_data_.root().get(kLaunchCount, Json::Value(0)).asInt() >
        kLaunchCountBeforeAd)
      Engine::Get().ShowInterstitialAd();
    if (state_ == kGame)
      EnterMenuState();
  }
}

void Demo::AddScore(size_t score) {
  delta_score_ += score;
  wave_score_ += score;
}

void Demo::SetEnableMusic(bool enable) {
  if (enable) {
    if (boss_fight_)
      boss_music_.Resume(1);
    else
      music_.Resume(1);
  } else {
    music_.Stop(1);
    boss_music_.Stop(1);
  }
}

void Demo::EnterMenuState() {
  saved_data_.Save();

  if (state_ == kMenu)
    return;

  player_.OnInputEvent(
      std::make_unique<InputEvent>(InputEvent::kDragCancel, (size_t)0));
  player_.OnInputEvent(
      std::make_unique<InputEvent>(InputEvent::kDragCancel, (size_t)1));

  Dimmer(true);
  if (state_ == kState_Invalid || state_ == kGame) {
    hud_.Pause(true);
    player_.Pause(true);
    enemy_.Pause(true);
  }

  if (state_ == kState_Invalid || state_ == kGameOver) {
    menu_.SetOptionEnabled(Menu::kContinue, false);
    menu_.SetOptionEnabled(Menu::kNewGame, true);
  } else if (state_ == kGame) {
    menu_.SetOptionEnabled(Menu::kContinue, true);
    menu_.SetOptionEnabled(Menu::kNewGame, false);
  }
  menu_.Show();
  state_ = kMenu;
}

void Demo::EnterCreditsState() {
  if (state_ == kCredits)
    return;

  credits_.Show();
  state_ = kCredits;
}

void Demo::EnterGameState() {
  if (state_ == kGame)
    return;

  Dimmer(false);
  sky_.SetSpeed(0.04f);

  hud_.Show();
  hud_.Pause(false);
  player_.Pause(false);
  enemy_.Pause(false);
  if (boss_fight_)
    hud_.HideProgress();
  state_ = kGame;
}

void Demo::EnterGameOverState() {
  if (state_ == kGameOver)
    return;

  saved_data_.Save();

  enemy_.PauseProgress();
  enemy_.StopAllEnemyUnits();
  sky_.SwitchColor({0, 0, 0, 1});
  hud_.ShowMessage("Game Over", 3);
  state_ = kGameOver;

  if (boss_fight_) {
    music_.Resume(10);
    boss_music_.Stop(10);
  }

  SetDelayedWork(1, [&]() -> void {
    enemy_.RemoveAll();
    // hud_.Hide();
    SetDelayedWork(3, [&]() -> void {
      wave_ = 0;
      boss_fight_ = false;
      EnterMenuState();
    });
  });

#if defined(RECORD)
  Engine::Get().EndRecording("replay");
#endif
}

void Demo::UpdateMenuState(float delta_time) {
  switch (menu_.selected_option()) {
    case Menu::kOption_Invalid:
      break;
    case Menu::kContinue:
      menu_.Hide();
      Continue();
      break;
    case Menu::kNewGame:
      menu_.Hide([&]() {
        if (saved_data_.root().get(kLaunchCount, Json::Value(0)).asInt() >
            kLaunchCountBeforeAd)
          Engine::Get().ShowInterstitialAd();
        StartNewGame();
      });
      break;
    case Menu::kCredits:
      menu_.Hide();
      EnterCreditsState();
      break;
    case Menu::kExit:
      Engine::Get().Exit();
      break;
    default:
      NOTREACHED << "- Unknown menu option: " << menu_.selected_option();
  }
}

void Demo::UpdateGameState(float delta_time) {
  if (delayed_work_timer_ > 0) {
    delayed_work_timer_ -= delta_time;
    if (delayed_work_timer_ <= 0) {
      base::Closure cb = std::move(delayed_work_cb_);
      delayed_work_cb_ = nullptr;
      cb();
    }
  }

  if (delta_score_ > 0) {
    total_score_ += delta_score_;
    delta_score_ = 0;
    hud_.SetScore(total_score_, true);

    if (total_score_ > GetHighScore())
      saved_data_.root()[kHightScore] = total_score_;
  }

  if (wave_ > saved_data_.root().get(kLastWave, Json::Value(0)).asInt())
    saved_data_.root()[kLastWave] = wave_;

  sky_.Update(delta_time);
  player_.Update(delta_time);
  enemy_.Update(delta_time);

  if (waiting_for_next_wave_)
    return;

  if (boss_fight_) {
    if (!enemy_.IsBossAlive())
      StartNextStage(false);
  } else if (enemy_.num_enemies_killed_in_current_wave() !=
             last_num_enemies_killed_) {
    bool no_boss = (last_num_enemies_killed_ == -1);
    if (last_num_enemies_killed_ < enemy_.num_enemies_killed_in_current_wave())
      last_num_enemies_killed_ = enemy_.num_enemies_killed_in_current_wave();
    int enemies_remaining = total_enemies_ - last_num_enemies_killed_;

    if (enemies_remaining <= 0)
      StartNextStage(wave_ && !(wave_ % 3) && !no_boss);
    else
      hud_.SetProgress((float)enemies_remaining / (float)total_enemies_);
  }
}

void Demo::Continue() {
  EnterGameState();
}

void Demo::StartNewGame() {
#if defined(RECORD)
  Json::Value game_data;
  game_data["wave"] = RECORD;
  wave_ = RECORD - 1;
  Engine::Get().StartRecording(game_data);
#elif defined(REPLAY)
  Json::Value game_data;
  Engine::Get().Replay("replay", game_data);
  wave_ = game_data["wave"].asInt() - 1;
#else
  wave_ = menu_.start_from_wave() - 1;
#endif

  wave_score_ = total_score_ = delta_score_ = 0;
  last_num_enemies_killed_ = -1;
  total_enemies_ = 0;
  waiting_for_next_wave_ = false;
  boss_fight_ = false;
  delayed_work_timer_ = 0;
  delayed_work_cb_ = nullptr;
  player_.Reset();
  enemy_.Reset();
  EnterGameState();
}

void Demo::StartNextStage(bool boss) {
  waiting_for_next_wave_ = true;
  hud_.SetProgress(wave_ > 0 ? 0 : 1);

  DLOG_IF(wave_ > 0 && stage_time_ > 0)
      << "wave: " << wave_ << " time: " << stage_time_ / 60.0f;
  stage_time_ = 0;

  enemy_.PauseProgress();
  enemy_.StopAllEnemyUnits();

  if (boss) {
    boss_music_.Play(true, 10);
    music_.Stop(10);
  } else if (boss_fight_) {
    music_.Resume(10);
    boss_music_.Stop(10);
  }

  SetDelayedWork(1.25f, [&, boss]() -> void {
    enemy_.KillAllEnemyUnits();

    SetDelayedWork(boss_fight_ ? 4 : 0.5f, [&, boss]() -> void {
      if (boss) {
        sky_.SwitchColor(sky_.nebula_color() * 0.5f);

        hud_.HideProgress();

        boss_fight_ = true;
        DLOG << "Boss fight.";
      } else {
        size_t bonus_factor = [&]() -> size_t {
          if (wave_ <= 3)
            return 2;
          if (wave_ <= 6)
            return 5;
          if (wave_ <= 9)
            return 15;
          return 100;
        }();
        size_t bonus_score = wave_score_ * (bonus_factor - 1);
        DLOG << "total_score_" << total_score_ << " wave " << wave_
             << " score: " << wave_score_ << " bonus: " << bonus_score;

        if (bonus_score > 0) {
          delta_score_ += bonus_score;
          hud_.ShowBonus(bonus_score);
          wave_score_ = 0;
        }

        Random& rnd = Engine::Get().GetRandomGenerator();
        int dominant_channel = rnd.Roll(3) - 1;
        if (dominant_channel == last_dominant_channel_)
          dominant_channel = (dominant_channel + 1) % 3;
        last_dominant_channel_ = dominant_channel;

        float weights[3] = {0, 0, 0};
        weights[dominant_channel] = 1;
        Vector4f c = {Lerp(0.75f, 0.95f, rnd.GetFloat()) * weights[0],
                      Lerp(0.75f, 0.95f, rnd.GetFloat()) * weights[1],
                      Lerp(0.75f, 0.95f, rnd.GetFloat()) * weights[2], 1};
        c += {Lerp(0.1f, 0.5f, rnd.GetFloat()) * (1 - weights[0]),
              Lerp(0.1f, 0.5f, rnd.GetFloat()) * (1 - weights[1]),
              Lerp(0.1f, 0.5f, rnd.GetFloat()) * (1 - weights[2]), 1};
        sky_.SwitchColor(c);

        ++wave_;
        hud_.Show();
        hud_.SetProgress(1);

        if (boss_fight_)
          player_.TakeDamage(-3);

        total_enemies_ = 20.0f + 23.0897f * log((float)wave_);
        last_num_enemies_killed_ = 0;
        boss_fight_ = false;
        DLOG << "wave: " << wave_ << " total_enemies_: " << total_enemies_;
      }

      hud_.SetScore(total_score_, true);
      hud_.SetWave(wave_, true);

      enemy_.OnWaveStarted(wave_, boss);

      waiting_for_next_wave_ = false;
    });
  });
}

void Demo::Win() {
  // Satisfy win conditions.
  if (boss_fight_)
    enemy_.KillBoss();
  else
    last_num_enemies_killed_ = total_enemies_;
}

void Demo::Dimmer(bool enable) {
  if (enable && !dimmer_active_) {
    dimmer_active_ = true;
    dimmer_.SetColor(kBgColor * Vector4f(0, 0, 0, 0));
    dimmer_animator_.SetBlending(kBgColor, kFadeSpeed);
    dimmer_animator_.Play(Animator::kBlending, false);
    dimmer_animator_.SetVisible(true);
  } else if (!enable && dimmer_active_) {
    dimmer_active_ = false;
    dimmer_animator_.SetBlending(kBgColor * Vector4f(0, 0, 0, 0), kFadeSpeed);
    dimmer_animator_.Play(Animator::kBlending, false);
    dimmer_animator_.SetEndCallback(Animator::kBlending, [&]() -> void {
      dimmer_animator_.SetEndCallback(Animator::kBlending, nullptr);
      dimmer_animator_.SetVisible(false);
    });
  }
}

size_t Demo::GetHighScore() const {
  return saved_data_.root().get(kHightScore, Json::Value(0)).asUInt64();
}

void Demo::SetDelayedWork(float seconds, base::Closure cb) {
  DCHECK(delayed_work_cb_ == nullptr);
  delayed_work_cb_ = std::move(cb);
  delayed_work_timer_ = seconds;
}

void Demo::BenchmarkResult(int avarage_fps) {
  LOG << __func__ << " avarage_fps: " << avarage_fps;
  if (avarage_fps < 30)
    sky_.Create(true);
}
