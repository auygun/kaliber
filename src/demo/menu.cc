#include "menu.h"

#include <cmath>
#include <vector>

#include "../base/collusion_test.h"
#include "../base/interpolation.h"
#include "../base/log.h"
#include "../engine/engine.h"
#include "../engine/font.h"
#include "../engine/image.h"
#include "../engine/input_event.h"
#include "demo.h"

using namespace base;
using namespace eng;

namespace {

constexpr char kMenuOption[Menu::kOption_Max][10] = {"continue", "start",
                                                     "credits", "exit"};

constexpr float kMenuOptionSpace = 1.5f;

const Vector4f kColorNormal = {1, 1, 1, 1};
const Vector4f kColorHighlight = {5, 5, 5, 1};
constexpr float kBlendingSpeed = 0.12f;

const Vector4f kColorFadeOut = {1, 1, 1, 0};
constexpr float kFadeSpeed = 0.2f;

}  // namespace

Menu::Menu() = default;

Menu::~Menu() = default;

bool Menu::Initialize() {
  const Font& font = static_cast<Demo*>(Engine::Get().GetGame())->GetFont();

  max_text_width_ = -1;
  for (int i = 0; i < kOption_Max; ++i) {
    int width, height;
    font.CalculateBoundingBox(kMenuOption[i], width, height);
    if (width > max_text_width_)
      max_text_width_ = width;
  }

  if (!CreateRenderResources())
    return false;

  for (int i = 0; i < kOption_Max; ++i) {
    items_[i].text.Create("menu_tex", {1, 4});
    items_[i].text.SetZOrder(40);
    items_[i].text.Scale(1.5f);
    items_[i].text.SetColor(kColorFadeOut);
    items_[i].text.SetVisible(false);
    items_[i].text.SetFrame(i);

    items_[i].select_item_cb_ = [&, i]() -> void {
      items_[i].text_animator.SetEndCallback(
          Animator::kBlending, [&, i]() -> void {
            items_[i].text_animator.SetEndCallback(Animator::kBlending,
                                                   nullptr);
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

void Menu::OnInputEvent(std::unique_ptr<InputEvent> event) {
  if (event->GetType() == InputEvent::kDragStart) {
    tap_pos_[0] = event->GetVector();
    tap_pos_[1] = event->GetVector();
  } else if (event->GetType() == InputEvent::kDrag) {
    tap_pos_[1] = event->GetVector();
  }

  if (event->GetType() != InputEvent::kDragEnd || IsAnimating())
    return;

  for (int i = 0; i < kOption_Max; ++i) {
    if (items_[i].hide)
      continue;
    if (!Intersection(items_[i].text.GetPosition(),
                      items_[i].text.GetSize() * Vector2f(1.2f, 2),
                      tap_pos_[0]))
      continue;
    if (!Intersection(items_[i].text.GetPosition(),
                      items_[i].text.GetSize() * Vector2f(1.2f, 2),
                      tap_pos_[1]))
      continue;

    items_[i].text_animator.SetEndCallback(Animator::kBlending,
                                           items_[i].select_item_cb_);
    items_[i].text_animator.SetBlending(kColorHighlight, kBlendingSpeed);
    items_[i].text_animator.Play(Animator::kBlending, false);
  }
}

void Menu::SetOptionEnabled(Option o, bool enable) {
  int first = -1, last = -1;
  for (int i = 0; i < kOption_Max; ++i) {
    if (i == o)
      items_[i].hide = !enable;
    if (!items_[i].hide) {
      items_[i].text.SetPosition({0, 0});
      if (last >= 0) {
        items_[i].text.PlaceToBottomOf(items_[last].text);
        items_[i].text.Translate(items_[last].text.GetPosition() *
                                 Vector2f(0, 1));
        items_[i].text.Translate(
            {0, items_[last].text.GetSize().y * -kMenuOptionSpace});
      }
      if (first < 0)
        first = i;
      last = i;
    }
  }

  float center_offset_y =
      (items_[first].text.GetPosition().y - items_[last].text.GetPosition().y) /
      2;
  for (int i = 0; i < kOption_Max; ++i) {
    if (!items_[i].hide)
      items_[i].text.Translate({0, center_offset_y});
  }
}

void Menu::Show() {
  for (int i = 0; i < kOption_Max; ++i) {
    if (items_[i].hide)
      continue;
    items_[i].text_animator.SetEndCallback(
        Animator::kBlending, [&, i]() -> void {
          items_[i].text_animator.SetEndCallback(Animator::kBlending, nullptr);
        });
    items_[i].text_animator.SetBlending(kColorNormal, kFadeSpeed);
    items_[i].text_animator.Play(Animator::kBlending, false);
    items_[i].text.SetVisible(true);
  }
}

void Menu::Hide() {
  selected_option_ = kOption_Invalid;
  for (int i = 0; i < kOption_Max; ++i) {
    if (items_[i].hide)
      continue;
    items_[i].text_animator.SetEndCallback(
        Animator::kBlending, [&, i]() -> void {
          items_[i].text_animator.SetEndCallback(Animator::kBlending, nullptr);
          items_[i].text.SetVisible(false);
        });
    items_[i].text_animator.SetBlending(kColorFadeOut, kFadeSpeed);
    items_[i].text_animator.Play(Animator::kBlending, false);
  }
}

bool Menu::CreateRenderResources() {
  Engine::Get().SetImageSource("menu_tex", std::bind(&Menu::CreateImage, this));

  return true;
}

std::unique_ptr<Image> Menu::CreateImage() {
  const Font& font = static_cast<Demo*>(Engine::Get().GetGame())->GetFont();

  int line_height = font.GetLineHeight() + 1;
  auto image = std::make_unique<Image>();
  image->Create(max_text_width_, line_height * kOption_Max);

  // Fill the area of each menu item with gradient.
  image->GradientV({1.0f, 1.0f, 1.0f, 0}, {.0f, .0f, 1.0f, 0}, line_height);

  for (int i = 0; i < kOption_Max; ++i) {
    int w, h;
    font.CalculateBoundingBox(kMenuOption[i], w, h);
    float x = (image->GetWidth() - w) / 2;
    float y = line_height * i;
    font.Print(x, y, kMenuOption[i], image->GetBuffer(), image->GetWidth());
  }

  return image;
}

bool Menu::IsAnimating() {
  for (int i = 0; i < kOption_Max; ++i) {
    if (items_[i].text_animator.IsPlaying(Animator::kBlending))
      return true;
  }
  return false;
}
