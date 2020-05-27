#include "menu.h"

#include <cassert>
#include <cmath>
#include <vector>

#include "../base/collusion_test.h"
#include "../base/interpolation.h"
#include "../base/log.h"
#include "../base/vecmath.h"
#include "../base/worker.h"
#include "../engine/engine.h"
#include "../engine/font.h"
#include "../engine/image.h"
#include "../engine/input_event.h"

using namespace base;
using namespace eng;

namespace {

constexpr char kMenuOption[Menu::kOption_Max][10] = {"continue",
                                                     "start",
                                                     "credits",
                                                     "exit"};

constexpr float kMenuOptionSpace = 1.5f;

const Vector4 kColorNormal = {1, 1, 1, 1};
const Vector4 kColorHighlight = {5, 5, 5, 1};
constexpr float kBlendingSpeed = 0.12f;

const Vector4 kColorFadeOut = {1, 1, 1, 0};
constexpr float kFadeSpeed = 0.2f;

}  // namespace

bool Menu::Initialize() {
  eng::Engine& engine = eng::Engine::Get();

  font_ = engine.GetFontAsset("PixelCaps!.ttf");
  if (!font_)
    return false;

  max_text_width_ = -1;
  for (int i = 0; i < kOption_Max; ++i) {
    int width, height;
    font_->CalculateBoundingBox(kMenuOption[i], width, height);
    if (width > max_text_width_)
      max_text_width_ = width;
  }

  auto image = CreateImage();

  for (int i = 0; i < kOption_Max; ++i) {
    items_[i].text.Create(image, {1, 4});
    items_[i].text.AutoScale();
    items_[i].text.SetColor(kColorFadeOut);
    items_[i].text.SetVisible(false);
    items_[i].text.SetFrame(i);

    items_[i].select_item_cb_ = [&, i]() -> void {
      items_[i].text_animator.SetEndCallback(Animator::kBlending,
          [&, i]() -> void {
            items_[i].text_animator.SetEndCallback(Animator::kBlending,
                                                   nullptr);
            fading_ = false;
            selected_option_ = (Option)i;
          });
      items_[i].text_animator.SetBlending(kColorNormal, kBlendingSpeed);
      items_[i].text_animator.Play(Animator::kBlending, false);
    };
    items_[i].text_animator.Attach(&items_[i].text);
  }
  // Get the item positions calculated.
  SetOptionEnabled(kContinue, true);

  return true;
}

void Menu::Update(float delta_time) {
  for (int i = 0; i < kOption_Max; ++i) {
    if (items_[i].hide)
      continue;
    items_[i].text_animator.Update(delta_time);
  }
}

void Menu::OnInputEvent(std::unique_ptr<eng::InputEvent> event) {
  if (event->GetType() != eng::InputEvent::kTap || fading_)
    return;

  for (int i = 0; i < kOption_Max; ++i) {
    if (items_[i].hide)
      continue;
    if (Intersection(items_[i].text.GetOffset(),
                     items_[i].text.GetScale() * Vector2(1.2f, 2),
                     event->GetVector(0))) {
      fading_ = true;
      items_[i].text_animator.SetEndCallback(Animator::kBlending,
                                             items_[i].select_item_cb_);
      items_[i].text_animator.SetBlending(kColorHighlight, kBlendingSpeed);
      items_[i].text_animator.Play(Animator::kBlending, false);
    }
  }
}

void Menu::Draw() {
  for (int i = 0; i < kOption_Max; ++i)
    items_[i].text.Draw();
}

void Menu::ContextLost() {
  auto image = CreateImage();
  for (int i = 0; i < kOption_Max; ++i) {
    items_[i].text.ContextLost();
    items_[i].text.Create(image, {1, 4});
  }
}

void Menu::SetOptionEnabled(Option o, bool enable) {
  int y = 0;
  for (int i = 0; i < kOption_Max; ++i) {
    if (i == o)
      items_[i].hide = !enable;
    if (!items_[i].hide) {
      Vector2 space = {0, items_[i].text.GetScale().y * kMenuOptionSpace};
      Vector2 pos = items_[i].text.GetScale() * -y +
                    (items_[i].text.GetScale() + space / 2) *
                    Vector2(0, (kOption_Max - 1) / 2) - space * y;
      ++y;
      items_[i].text.SetOffset(pos * Vector2(0, 1));
    }
  }
}

void Menu::Show() {
  fading_ = true;
  for (int i = 0; i < kOption_Max; ++i) {
    if (items_[i].hide)
      continue;
    items_[i].text_animator.SetEndCallback(Animator::kBlending, [&, i]() -> void {
          items_[i].text_animator.SetEndCallback(Animator::kBlending, nullptr);
          fading_ = false;
        });
    items_[i].text_animator.SetBlending(kColorNormal, kFadeSpeed);
    items_[i].text_animator.Play(Animator::kBlending, false);
    items_[i].text.SetVisible(true);
  }
}

void Menu::Hide() {
  fading_ = true;
  selected_option_ = kOption_Invalid;
  for (int i = 0; i < kOption_Max; ++i) {
    if (items_[i].hide)
      continue;
    items_[i].text_animator.SetEndCallback(Animator::kBlending, [&, i]() -> void {
          items_[i].text_animator.SetEndCallback(Animator::kBlending, nullptr);
          items_[i].text.SetVisible(false);
          fading_ = false;
        });
    items_[i].text_animator.SetBlending(kColorFadeOut, kFadeSpeed);
    items_[i].text_animator.Play(Animator::kBlending, false);
  }
}

std::shared_ptr<eng::Image> Menu::CreateImage() {
  int line_height = font_->GetLineHeight() + 1;
  auto image = std::make_shared<eng::Image>();
  image->Create(max_text_width_, line_height * kOption_Max);

  // Fill the area of each menu item with gradient.
  Vector4 c1 = {1.0f, 1.0f, 1.0f, 0};
  Vector4 c2 = {.0f, .0f, 1.0f, 0};
  uint8_t* buffer = image->GetBuffer();
  for (int h = 0; h < image->GetHeight(); ++h) {
    Vector4 c = Lerp(c1, c2, fmod(h, line_height) / (float)line_height);
    for (int x = 0; x < image->GetWidth(); ++x) {
      buffer[h * image->GetWidth() * 4 + x * 4 + 0] = c.x * 255;
      buffer[h * image->GetWidth() * 4 + x * 4 + 1] = c.y * 255;
      buffer[h * image->GetWidth() * 4 + x * 4 + 2] = c.z * 255;
      buffer[h * image->GetWidth() * 4 + x * 4 + 3] = 0;
    }
  }

  base::Worker worker(kOption_Max);
  for (int i = 0; i < kOption_Max; ++i) {
    int w, h;
    font_->CalculateBoundingBox(kMenuOption[i], w, h);
    float x = (image->GetWidth() - w) / 2;
    float y = line_height * i;
    worker.Enqueue(std::bind(&Font::Print, font_, x, y,
                             kMenuOption[i], image->GetBuffer(),
                             image->GetWidth()));
  }
  worker.Join();

  image->SetImmutable();
  return image;
}
