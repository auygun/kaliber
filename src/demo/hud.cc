#include "demo/hud.h"

#include "base/interpolation.h"
#include "base/log.h"
#include "base/vecmath.h"
#include "engine/asset/font.h"
#include "engine/asset/image.h"
#include "engine/engine.h"

#include "demo/demo.h"

using namespace std::string_literals;

using namespace base;
using namespace eng;

namespace {

constexpr float kHorizontalMargin = 0.07f;
constexpr float kVerticalMargin = 0.025f;

const Vector4f kPprogressBarColor[2] = {{0.256f, 0.434f, 0.72f, 1},
                                        {0.905f, 0.493f, 0.194f, 1}};
const Vector4f kTextColor = {0.895f, 0.692f, 0.24f, 1};

void SetupFadeOutAnim(Animator& animator, float delay) {
  animator.SetEndCallback(Animator::kTimer, [&]() -> void {
    animator.SetBlending({1, 1, 1, 0}, 0.5f,
                         std::bind(Acceleration, std::placeholders::_1, -1));
    animator.Play(Animator::kBlending, false);
  });
  animator.SetEndCallback(Animator::kBlending,
                          [&]() -> void { animator.SetVisible(false); });
  animator.SetTimer(delay);
}

}  // namespace

Hud::Hud() = default;

Hud::~Hud() = default;

bool Hud::Initialize() {
  Engine& engine = Engine::Get();
  const Font& font = static_cast<Demo*>(engine.GetGame())->GetFont();

  int tmp;
  font.CalculateBoundingBox("big_enough_text", max_text_width_, tmp);

  Engine::Get().SetImageSource("text0",
                               std::bind(&Hud::CreateScoreImage, this));
  Engine::Get().SetImageSource("text1", std::bind(&Hud::CreateWaveImage, this));
  Engine::Get().SetImageSource("message",
                               std::bind(&Hud::CreateMessageImage, this));
  Engine::Get().SetImageSource("bonus_tex",
                               std::bind(&Hud::CreateBonusImage, this));

  for (int i = 0; i < 2; ++i) {
    text_[i].Create("text"s + std::to_string(i));
    text_[i].SetZOrder(30);
    text_[i].SetColor(kTextColor * Vector4f(1, 1, 1, 0));

    Vector2f pos = (engine.GetScreenSize() / 2 - text_[i].GetSize() / 2);
    pos -=
        engine.GetScreenSize() * Vector2f(kHorizontalMargin, kVerticalMargin);

    Vector2f scale = engine.GetScreenSize() * Vector2f(1, 0);
    scale -= engine.GetScreenSize() * Vector2f(kHorizontalMargin * 4, 0);
    scale += text_[0].GetSize() * Vector2f(0, 0.3f);

    progress_bar_[i].SetZOrder(30);
    progress_bar_[i].SetSize(scale);
    progress_bar_[i].Translate(pos * Vector2f(0, 1));
    progress_bar_[i].SetColor(kPprogressBarColor[i] * Vector4f(1, 1, 1, 0));

    pos -= progress_bar_[i].GetSize() * Vector2f(0, 4);
    text_[i].Translate(pos * Vector2f(i ? 1 : -1, 1));

    progress_bar_animator_[i].Attach(&progress_bar_[i]);

    text_animator_cb_[i] = [&, i]() -> void {
      text_animator_[i].SetEndCallback(Animator::kBlending, nullptr);
      text_animator_[i].SetBlending(
          kTextColor, 2, std::bind(Acceleration, std::placeholders::_1, -1));
      text_animator_[i].Play(Animator::kBlending, false);
    };
    text_animator_[i].Attach(&text_[i]);
  }

  message_.Create("message");
  message_.SetZOrder(30);

  message_animator_.SetEndCallback(Animator::kTimer, [&]() -> void {
    message_animator_.SetEndCallback(Animator::kBlending, [&]() -> void {
      message_animator_.SetVisible(false);
    });
    message_animator_.SetBlending({1, 1, 1, 0}, 0.5f);
    message_animator_.Play(Animator::kBlending, false);
  });
  message_animator_.Attach(&message_);

  bonus_.Create("bonus_tex");
  bonus_.SetZOrder(30);

  SetupFadeOutAnim(bonus_animator_, 1.0f);
  bonus_animator_.SetMovement({0, Engine::Get().GetScreenSize().y / 2}, 2.0f);
  bonus_animator_.Attach(&bonus_);

  return true;
}

void Hud::Pause(bool pause) {
  message_animator_.PauseOrResumeAll(pause);
  bonus_animator_.PauseOrResumeAll(pause);
}

void Hud::Show() {
  if (text_[0].IsVisible() && text_[1].IsVisible() &&
      progress_bar_[0].IsVisible() && progress_bar_[1].IsVisible())
    return;

  for (int i = 0; i < 2; ++i) {
    progress_bar_[i].SetVisible(true);
    text_[i].SetVisible(true);
    progress_bar_animator_[i].SetBlending(kPprogressBarColor[i], 0.5f);
    progress_bar_animator_[i].Play(Animator::kBlending, false);
  }
}

void Hud::Hide() {
  if (!text_[0].IsVisible() && !text_[1].IsVisible() &&
      !progress_bar_[0].IsVisible() && !progress_bar_[1].IsVisible())
    return;

  for (int i = 0; i < 2; ++i) {
    text_animator_[i].SetEndCallback(Animator::kBlending, [&, i]() -> void {
      text_animator_[i].SetEndCallback(Animator::kBlending, nullptr);
      text_[i].SetVisible(false);
    });
    text_animator_[i].SetBlending(kTextColor * Vector4f(1, 1, 1, 0), 0.5f);
    text_animator_[i].Play(Animator::kBlending, false);
  }

  HideProgress();
}

void Hud::HideProgress() {
  if (!progress_bar_[0].IsVisible())
    return;

  for (int i = 0; i < 2; ++i) {
    progress_bar_animator_[i].SetEndCallback(
        Animator::kBlending, [&, i]() -> void {
          progress_bar_animator_[i].SetEndCallback(Animator::kBlending,
                                                   nullptr);
          progress_bar_animator_[1].SetVisible(false);
        });
    progress_bar_animator_[i].SetBlending(
        kPprogressBarColor[i] * Vector4f(1, 1, 1, 0), 0.5f);
    progress_bar_animator_[i].Play(Animator::kBlending, false);
  }
}

void Hud::SetScore(size_t score, bool flash) {
  last_score_ = score;
  Engine::Get().RefreshImage("text0");

  if (flash) {
    text_animator_[0].SetEndCallback(Animator::kBlending, text_animator_cb_[0]);
    text_animator_[0].SetBlending(
        {1, 1, 1, 1}, 0.1f, std::bind(Acceleration, std::placeholders::_1, 1));
    text_animator_[0].Play(Animator::kBlending, false);
  }
}

void Hud::SetWave(int wave, bool flash) {
  last_wave_ = wave;
  Engine::Get().RefreshImage("text1");

  if (flash) {
    text_animator_[1].SetEndCallback(Animator::kBlending, text_animator_cb_[1]);
    text_animator_[1].SetBlending(
        {1, 1, 1, 1}, 0.1f, std::bind(Acceleration, std::placeholders::_1, 1));
    text_animator_[1].Play(Animator::kBlending, false);
  }
}

void Hud::SetProgress(float progress) {
  progress = std::min(std::max(0.0f, progress), 1.0f);
  last_progress_ = progress;
  Vector2f s = progress_bar_[0].GetSize() * Vector2f(progress, 1);
  float t = (s.x - progress_bar_[1].GetSize().x) / 2;
  progress_bar_[1].SetSize(s);
  progress_bar_[1].Translate({t, 0});
}

void Hud::ShowMessage(const std::string& text, float duration) {
  message_text_ = text;
  Engine::Get().RefreshImage("message");

  message_.Scale(1.5f);
  message_.SetColor({1, 1, 1, 0});
  message_.SetVisible(true);

  message_animator_.SetEndCallback(
      Animator::kBlending, [&, duration]() -> void {
        message_animator_.SetTimer(duration);
        message_animator_.Play(Animator::kTimer, false);
      });
  message_animator_.SetBlending({1, 1, 1, 1}, 0.5f);
  message_animator_.Play(Animator::kBlending, false);
}

void Hud::ShowBonus(size_t bonus) {
  bonus_score_ = bonus;
  Engine::Get().RefreshImage("bonus_tex");
  bonus_.Scale(1.3f);
  bonus_.SetColor({1, 1, 1, 1});
  bonus_.SetVisible(true);
  bonus_animator_.Play(Animator::kTimer | Animator::kMovement, false);
}

std::unique_ptr<eng::Image> Hud::CreateScoreImage() {
  return Print(0, std::to_string(last_score_));
}

std::unique_ptr<eng::Image> Hud::CreateWaveImage() {
  return Print(1, "wave "s + std::to_string(last_wave_));
}

std::unique_ptr<Image> Hud::CreateMessageImage() {
  const Font& font = static_cast<Demo*>(Engine::Get().GetGame())->GetFont();

  auto image = std::make_unique<Image>();
  image->Create(max_text_width_, font.GetLineHeight());
  image->GradientV({0.80f, 0.87f, 0.93f, 0}, {0.003f, 0.91f, 0.99f, 0},
                   font.GetLineHeight());

  int w, h;
  font.CalculateBoundingBox(message_text_.c_str(), w, h);
  float x = (image->GetWidth() - w) / 2;

  font.Print(x, 0, message_text_.c_str(), image->GetBuffer(),
             image->GetWidth());
  image->Compress();
  return image;
}

std::unique_ptr<Image> Hud::CreateBonusImage() {
  const Font& font = static_cast<Demo*>(Engine::Get().GetGame())->GetFont();

  auto image = CreateImage();

  std::string text = std::to_string(bonus_score_);
  int w, h;
  font.CalculateBoundingBox(text.c_str(), w, h);
  float x = (image->GetWidth() - w) / 2;

  font.Print(x, 0, text.c_str(), image->GetBuffer(), image->GetWidth());
  image->Compress();
  return image;
}

std::unique_ptr<Image> Hud::Print(int i, const std::string& text) {
  const Font& font = static_cast<Demo*>(Engine::Get().GetGame())->GetFont();

  auto image = CreateImage();

  float x = 0;
  if (i == 1) {
    int w, h;
    font.CalculateBoundingBox(text.c_str(), w, h);
    x = image->GetWidth() - w;
  }

  font.Print(x, 0, text.c_str(), image->GetBuffer(), image->GetWidth());
  image->Compress();
  return image;
}

std::unique_ptr<Image> Hud::CreateImage() {
  const Font& font = static_cast<Demo*>(Engine::Get().GetGame())->GetFont();

  auto image = std::make_unique<Image>();
  image->Create(max_text_width_, font.GetLineHeight());
  image->Clear({1, 1, 1, 0});
  return image;
}
