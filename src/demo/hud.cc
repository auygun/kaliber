#include "hud.h"

#include "../base/interpolation.h"
#include "../base/log.h"
#include "../base/vecmath.h"
#include "../engine/engine.h"
#include "../engine/font.h"
#include "../engine/image.h"
#include "../engine/renderer/texture.h"

using namespace base;
using namespace eng;

namespace {

constexpr float kHorizontalMargin = 0.07f;
constexpr float kVerticalMargin = 0.025f;

const Vector4 kPprogressBarColor[2] = {{0.256f, 0.434f, 0.72f, 1},
                                       {0.905f, 0.493f, 0.194f, 1}};
const Vector4 kTextColor = {0.895f, 0.692f, 0.24f, 1};

}  // namespace

Hud::Hud() {
  text_[0].Create(Engine::Get().CreateRenderResource<Texture>());
  text_[1].Create(Engine::Get().CreateRenderResource<Texture>());
}

Hud::~Hud() = default;

bool Hud::Initialize() {
  Engine& engine = Engine::Get();

  font_ = engine.GetAsset<Font>("PixelCaps!.ttf");
  if (!font_)
    return false;

  int tmp;
  font_->CalculateBoundingBox("big_enough_text", max_text_width_, tmp);

  auto image = CreateImage();
  image->SetImmutable();

  for (int i = 0; i < 2; ++i) {
    text_[i].GetTexture()->Update(image);
    text_[i].AutoScale();
    text_[i].SetColor(kTextColor);

    Vector2 pos = (engine.GetScreenSize() / 2 - text_[i].GetScale() / 2);
    pos -= engine.GetScreenSize() * Vector2(kHorizontalMargin, kVerticalMargin);

    Vector2 scale = engine.GetScreenSize() * Vector2(1, 0);
    scale -= engine.GetScreenSize() * Vector2(kHorizontalMargin * 4, 0);
    scale += text_[0].GetScale() * Vector2(0, 0.3f);

    progress_bar_[i].Scale(scale);
    progress_bar_[i].Translate(pos * Vector2(0, 1));
    progress_bar_[i].SetColor(kPprogressBarColor[i] * Vector4(1, 1, 1, 0));

    pos -= progress_bar_[i].GetScale() * Vector2(0, 4);
    text_[i].Translate(pos * Vector2(i ? 1 : -1, 1));

    progress_bar_animator_[i].Attach(&progress_bar_[i]);

    text_animator_cb_[i] = [&, i]() -> void {
      text_animator_[i].SetEndCallback(Animator::kBlending, nullptr);
      text_animator_[i].SetBlending(
          kTextColor, 2, std::bind(Acceleration, std::placeholders::_1, -1));
      text_animator_[i].Play(Animator::kBlending, false);
    };
    text_animator_[i].Attach(&text_[i]);
  }

  return true;
}

void Hud::Update(float delta_time) {
  for (int i = 0; i < 2; ++i) {
    text_animator_[i].Update(delta_time);
    progress_bar_animator_[i].Update(delta_time);
  }
}

void Hud::Draw() {
  for (int i = 0; i < 2; ++i) {
    progress_bar_[i].Draw();
    text_[i].Draw();
  }
}

void Hud::ContextLost() {
  PrintScore(last_score_, false);
  PrintWave(last_wave_, false);
}

void Hud::Show() {
  if (text_[0].IsVisible())
    return;

  for (int i = 0; i < 2; ++i) {
    progress_bar_[i].SetVisible(true);
    text_[i].SetVisible(true);
    progress_bar_animator_[i].SetBlending(kPprogressBarColor[i], 0.3f);
    progress_bar_animator_[i].Play(Animator::kBlending, false);
  }
}

void Hud::PrintScore(int score, bool flash) {
  last_score_ = score;
  Print(0, std::to_string(score));

  if (flash) {
    text_animator_[0].SetEndCallback(Animator::kBlending, text_animator_cb_[0]);
    text_animator_[0].SetBlending(
        {1, 1, 1, 1}, 0.1f, std::bind(Acceleration, std::placeholders::_1, 1));
    text_animator_[0].Play(Animator::kBlending, false);
  }
}

void Hud::PrintWave(int wave, bool flash) {
  last_wave_ = wave;
  std::string text = "wave ";
  text += std::to_string(wave);
  Print(1, text.c_str());

  if (flash) {
    text_animator_[1].SetEndCallback(Animator::kBlending, text_animator_cb_[1]);
    text_animator_[1].SetBlending({1, 1, 1, 1}, 0.08f);
    text_animator_[1].Play(Animator::kBlending, false);
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

  text_[i].GetTexture()->Update(image);
}

std::shared_ptr<Image> Hud::CreateImage() {
  auto image = std::make_shared<Image>();
  image->Create(max_text_width_, font_->GetLineHeight());
  image->Clear({1, 1, 1, 0});
  return image;
}
