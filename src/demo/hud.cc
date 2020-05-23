#include "hud.h"

#include "../base/log.h"
#include "../base/vecmath.h"
#include "../engine/engine.h"
#include "../engine/font.h"
#include "../engine/image.h"

using base::Vector2;
using base::Vector4;

namespace {

constexpr float horizontal_margin = 0.07f;
constexpr float vertical_margin = 0.025f;
const Vector4 progress_bar_color[2] = {{0.256f, 0.434f, 0.72f, 1},
                                       {0.905f, 0.493f, 0.194f, 1}};

}  // namespace

bool Hud::Initialize() {
  eng::Engine& engine = eng::Engine::Get();

  font_ = engine.GetFontAsset("PixelCaps!.ttf");
  if (!font_)
    return false;

  int tmp;
  font_->CalculateBoundingBox("big_enough_text", max_text_width_, tmp);

  auto image = CreateImage();
  image->SetImmutable();

  for (int i = 0; i < 2; ++i) {
    text_[i].Create(image);
    text_[i].AutoScale();
    text_[i].SetColor({0.895f, 0.692f, 0.24f, 1});
    text_[i].SetVisible(true);

    Vector2 pos = (engine.GetScreenSize() / 2 - text_[i].GetScale() / 2);
    pos -= engine.GetScreenSize() * Vector2(horizontal_margin, vertical_margin);

    Vector2 scale = engine.GetScreenSize() * Vector2(1, 0);
    scale -= engine.GetScreenSize() * Vector2(horizontal_margin * 4, 0);
    scale += text_[0].GetScale() * Vector2(0, 0.3f);

    progress_bar_[i].Scale(scale);
    progress_bar_[i].Translate(pos * Vector2(0, 1));
    progress_bar_[i].SetColor(progress_bar_color[i]);

    pos -= progress_bar_[i].GetScale() * Vector2(0, 4);
    text_[i].Translate(pos * Vector2(i ? 1 : -1, 1));
  }

  hud_animator_cb_ = [&]() -> void {
    hud_animator_.SetEndCallback(eng::Animator::kBlending, nullptr);
    hud_animator_.SetBlending({0.895f, 0.692f, 0.24f, 1}, 0.2f);
    hud_animator_.Play(eng::Animator::kBlending, false);
  };
  hud_animator_.Attach(&text_[0]);

  return true;
}

void Hud::Update(float delta_time) {
  hud_animator_.Update(delta_time);
}

void Hud::Draw() {
  for (int i = 0; i < 2; ++i) {
    if (progress_bar_[i].IsVisible())
      progress_bar_[i].Draw();
    if (text_[i].IsVisible())
      text_[i].Draw();
  }
}

void Hud::ContextLost() {
  for (int i = 0; i < 2; ++i)
    text_[i].ContextLost();
  PrintScore(last_score_, false);
  PrintWave(last_wave_);
  SetProgress(last_progress_);
}

void Hud::PrintScore(int score, bool flash) {
  last_score_ = score;
  Print(0, std::to_string(score));

  if (flash) {
    hud_animator_.SetEndCallback(eng::Animator::kBlending, hud_animator_cb_);
    hud_animator_.SetBlending({1, 1, 1, 1}, 0.08f);
    hud_animator_.Play(eng::Animator::kBlending, false);
  }
}

void Hud::PrintWave(int wave) {
  last_wave_ = wave;
  std::string text = "wave ";
  text += std::to_string(wave);
  Print(1, text.c_str());

  if (!progress_bar_[0].IsVisible()) {
    progress_bar_[0].SetVisible(true);
    progress_bar_[1].SetVisible(true);
  }
}

void Hud::SetProgress(float progress) {
  progress = std::min(std::max(0.0f, progress), 1.0f);
  last_progress_ = progress;
  Vector2 s = progress_bar_[0].GetScale() * Vector2(progress, 1);
  float t = (s.x - progress_bar_[1].GetScale().x) / 2;
  progress_bar_[1].SetScale(s);
  progress_bar_[1].Translate({t, 0});
}

void Hud::Print(int i, const std::string& text) {
  auto image = CreateImage();

  float x = 0;
  if (i == 1) {
    int w, h;
    font_->CalculateBoundingBox(text.c_str(), w, h);
    x = image->GetWidth() - w;
  }

  font_->Print(x, 0, text.c_str(), image->GetBuffer(), image->GetWidth());
  image->SetImmutable();

  text_[i].Create(image);
}

std::shared_ptr<eng::Image> Hud::CreateImage() {
  auto image = std::make_shared<eng::Image>();
  image->Create(max_text_width_, font_->GetLineHeight());
  image->Clear({1, 1, 1, 0});
  return image;
}
