#include "credits.h"

#include "../base/log.h"
#include "../base/vecmath.h"
#include "../base/worker.h"
#include "../engine/engine.h"
#include "../engine/font.h"
#include "../engine/image.h"
#include "../engine/input_event.h"
#include "demo.h"

using namespace base;
using namespace eng;

namespace {

constexpr int kNumCreditsLines = 7;
constexpr char kCreditsLines[kNumCreditsLines][15] = {"Credits",
                                                      " ",
                                                      "Code:",
                                                      "Attila Uygun",
                                                      " ",
                                                      "Graphics:",
                                                      "Erkan Erturk"};

const Vector4 kTextColor = {0.3f, 0.55f, 1.0f, 1};
constexpr float kFadeSpeed = 0.2f;

}  // namespace

bool Credits::Initialize() {
  Engine& engine = Engine::Get();

  font_ = engine.GetFontAsset("PixelCaps!.ttf");
  if (!font_)
    return false;

  max_text_width_ = -1;
  for (int i = 0; i < kNumCreditsLines; ++i) {
    int width, height;
    font_->CalculateBoundingBox(kCreditsLines[i], width, height);
    if (width > max_text_width_)
      max_text_width_ = width;
  }

  auto image = CreateImage();
  image->SetImmutable();

  text_.Create(image);
  text_.AutoScale();
  text_.SetColor(kTextColor * Vector4(1, 1, 1, 0));

  text_animator_.Attach(&text_);
  return true;
}

void Credits::Update(float delta_time) {
  text_animator_.Update(delta_time);
}

void Credits::OnInputEvent(std::unique_ptr<eng::InputEvent> event) {
  if ((event->GetType() == eng::InputEvent::kTap ||
      event->GetType() == InputEvent::kNavigateBack) &&
      !text_animator_.IsPlaying(Animator::kBlending)) {
    Hide();
    Engine& engine = Engine::Get();
    static_cast<Demo*>(engine.GetGame())->EnterMenuState();
  }
}

void Credits::Draw() {
  text_.Draw();
}

void Credits::ContextLost() {
  text_.ContextLost();
  text_.Create(CreateImage());
}

void Credits::Show() {
  text_animator_.SetEndCallback(Animator::kBlending, [&]() -> void {
        text_animator_.SetEndCallback(Animator::kBlending, nullptr);
      });
  text_animator_.SetBlending(kTextColor, kFadeSpeed);
  text_animator_.Play(Animator::kBlending, false);
  text_.SetVisible(true);
}

void Credits::Hide() {
  text_animator_.SetEndCallback(Animator::kBlending, [&]() -> void {
        text_animator_.SetEndCallback(Animator::kBlending, nullptr);
        text_.SetVisible(false);
      });
  text_animator_.SetBlending(kTextColor * Vector4(1, 1, 1, 0), kFadeSpeed);
  text_animator_.Play(Animator::kBlending, false);
}

std::shared_ptr<eng::Image> Credits::CreateImage() {
  int margin = max_text_width_ / 10;
  int line_height = font_->GetLineHeight() + 1;
  int image_width = max_text_width_ + margin * 2;
  int image_height = (line_height + margin) * kNumCreditsLines + margin;

  auto image = std::make_shared<eng::Image>();
  image->Create(image_width, image_height);
  image->Clear({1, 1, 1, 0});

  Worker worker(kNumCreditsLines);
  int y = margin;
  for (int i = 0; i < kNumCreditsLines; ++i) {
    int w, h;
    font_->CalculateBoundingBox(kCreditsLines[i], w, h);
    float x = (image->GetWidth() - w) / 2;
    worker.Enqueue(std::bind(&Font::Print, font_, x, y,
                             kCreditsLines[i], image->GetBuffer(),
                             image->GetWidth()));
    y += line_height + margin;
  }
  worker.Join();

  image->SetImmutable();
  return image;
}
