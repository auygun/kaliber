#include "menu.h"

#include <cassert>
#include <cmath>
#include <vector>

#include "../base/collusion_test.h"
#include "../base/interpolation.h"
#include "../base/log.h"
#include "../base/worker.h"
#include "../engine/engine.h"
#include "../engine/font.h"
#include "../engine/image.h"
#include "../engine/input_event.h"
#include "../engine/sound.h"
#include "../engine/renderer/texture.h"
#include "demo.h"

using namespace base;
using namespace eng;

namespace {

constexpr char kMenuOption[Menu::kOption_Max][10] = {"continue", "start",
                                                     "credits", "exit"};

constexpr float kMenuOptionSpace = 1.5f;

const Vector4 kColorNormal = {1, 1, 1, 1};
const Vector4 kColorHighlight = {5, 5, 5, 1};
constexpr float kBlendingSpeed = 0.12f;

const Vector4 kColorFadeOut = {1, 1, 1, 0};
constexpr float kFadeSpeed = 0.2f;

}  // namespace

Menu::Menu() : logo_tex_{Engine::Get().CreateRenderResource<Texture>(),
                         Engine::Get().CreateRenderResource<Texture>()}
             , menu_tex_(Engine::Get().CreateRenderResource<Texture>()) {}

Menu::~Menu() = default;

bool Menu::Initialize() {
  click_sound_ = std::make_shared<Sound>();
  if (!click_sound_->Load("menu_click.mp3", false))
    return false;

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
    items_[i].text.Create(menu_tex_, {1, 4});
    items_[i].text.AutoScale();
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

  click_.SetSound(click_sound_);
  click_.SetVariate(false);
  click_.SetSimulateStereo(false);
  click_.SetMaxAplitude(1.5f);

  logo_[0].Create(logo_tex_[0], {3, 8});
  logo_[0].AutoScale();
  logo_[0].SetOffset(Engine::Get().GetScreenSize() * Vector2(0, 0.25f));

  logo_[1].Create(logo_tex_[1], {3, 7});
  logo_[1].AutoScale();
  logo_[1].SetOffset(Engine::Get().GetScreenSize() * Vector2(0, 0.25f));

  logo_animator_[0].Attach(&logo_[0]);
  logo_animator_[0].SetFrames(24, 20);
  logo_animator_[0].SetEndCallback(Animator::kFrames, [&]() -> void {
    logo_[0].SetVisible(false);
    logo_[1].SetVisible(true);
    logo_animator_[1].SetFrames(12, 20);
    logo_animator_[1].SetTimer(Lerp(3.0f, 8.0f, Engine::Get().GetRandomGenerator().GetFloat()));
    logo_animator_[1].Play(Animator::kFrames | Animator::kTimer, true);
  });

  logo_animator_[1].Attach(&logo_[1]);
  logo_animator_[1].SetEndCallback(Animator::kTimer, [&]() -> void {
    logo_animator_[1].Stop(Animator::kFrames);
    logo_[1].SetFrame(12);
    logo_animator_[1].SetFrames(9, 30);
    logo_animator_[1].SetTimer(Lerp(3.0f, 8.0f, Engine::Get().GetRandomGenerator().GetFloat()));
    logo_animator_[1].Play(Animator::kFrames | Animator::kTimer, false);
  });
  logo_animator_[1].SetEndCallback(Animator::kFrames, [&]() -> void {
    logo_[1].SetFrame(0);
    logo_animator_[1].SetFrames(12, 20);
    logo_animator_[1].Play(Animator::kFrames, true);
  });

  return true;
}

void Menu::Update(float delta_time) {
  for (int i = 0; i < 2; ++i)
    logo_animator_[i].Update(delta_time);

  for (int i = 0; i < kOption_Max; ++i) {
    if (items_[i].hide)
      continue;
    items_[i].text_animator.Update(delta_time);
  }
}

void Menu::OnInputEvent(std::unique_ptr<InputEvent> event) {
  if (event->GetType() == InputEvent::kDragStart)
    tap_pos_[0] = tap_pos_[1] = event->GetVector(0);
  else if (event->GetType() == InputEvent::kDrag)
    tap_pos_[1] = event->GetVector(0);

  if (event->GetType() != InputEvent::kDragEnd || IsAnimating())
    return;

  for (int i = 0; i < kOption_Max; ++i) {
    if (items_[i].hide)
      continue;
    if (!Intersection(items_[i].text.GetOffset(),
                     items_[i].text.GetScale() * Vector2(1.2f, 2),
                     tap_pos_[0]))
      continue;
    if (!Intersection(items_[i].text.GetOffset(),
                     items_[i].text.GetScale() * Vector2(1.2f, 2),
                     tap_pos_[1]))
      continue;

    items_[i].text_animator.SetEndCallback(Animator::kBlending,
                                            items_[i].select_item_cb_);
    items_[i].text_animator.SetBlending(kColorHighlight, kBlendingSpeed);
    items_[i].text_animator.Play(Animator::kBlending, false);

    click_.Play(false);
  }
}

void Menu::Draw() {
  logo_[0].Draw();
  logo_[1].Draw();

  for (int i = 0; i < kOption_Max; ++i)
    items_[i].text.Draw();
}

void Menu::ContextLost() {
  CreateRenderResources();
}

void Menu::SetOptionEnabled(Option o, bool enable) {
  int first = -1, last = -1;
  for (int i = 0; i < kOption_Max; ++i) {
    if (i == o)
      items_[i].hide = !enable;
    if (!items_[i].hide) {
      items_[i].text.SetOffset({0, 0});
      if (last >= 0) {
        items_[i].text.PlaceToBottomOf(items_[last].text);
        items_[i].text.Translate(items_[last].text.GetOffset() * Vector2(0, 1));
        items_[i].text.Translate(
            {0, items_[last].text.GetScale().y * -kMenuOptionSpace});
      }
      if (first < 0)
        first = i;
      last = i;
    }
  }

  float center_offset_y =
      (items_[first].text.GetOffset().y - items_[last].text.GetOffset().y) / 2;
  for (int i = 0; i < kOption_Max; ++i) {
    if (!items_[i].hide)
      items_[i].text.Translate({0, center_offset_y});
  }
}

void Menu::Show() {
  logo_[1].SetColor(kColorNormal);
  logo_animator_[0].SetVisible(true);
  logo_animator_[0].SetBlending(kColorNormal, kFadeSpeed);
  logo_animator_[0].Play(Animator::kBlending | Animator::kFrames, false);

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
  for (int i = 0; i < 2; ++i) {
    logo_animator_[i].SetBlending(kColorFadeOut, kFadeSpeed);
    logo_animator_[i].SetEndCallback(Animator::kBlending, [&, i]() -> void {
      logo_animator_[i].Stop(Animator::kAllAnimations | Animator::kTimer);
      logo_animator_[i].SetEndCallback(Animator::kBlending, nullptr);
      logo_animator_[i].SetVisible(false);
    });
    logo_animator_[i].Play(Animator::kBlending, false);
  }

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
  menu_tex_->Update(CreateImage());

  auto logo1_image = std::make_unique<Image>();
  if (!logo1_image->Load("woom_logo_start_frames_01.png"))
    return false;

  auto logo2_image = std::make_unique<Image>();
  if (!logo2_image->Load("woom_logo_start_frames_02-03.png"))
    return false;

  logo1_image->Compress();
  logo2_image->Compress();

  logo_tex_[0]->Update(std::move(logo1_image));
  logo_tex_[1]->Update(std::move(logo2_image));

  return true;
}

std::unique_ptr<Image> Menu::CreateImage() {
  const Font& font = static_cast<Demo*>(Engine::Get().GetGame())->GetFont();

  int line_height = font.GetLineHeight() + 1;
  auto image = std::make_unique<Image>();
  image->Create(max_text_width_, line_height * kOption_Max);

  // Fill the area of each menu item with gradient.
  image->GradientV({1.0f, 1.0f, 1.0f, 0}, {.0f, .0f, 1.0f, 0}, line_height);

  base::Worker worker(kOption_Max);
  for (int i = 0; i < kOption_Max; ++i) {
    int w, h;
    font.CalculateBoundingBox(kMenuOption[i], w, h);
    float x = (image->GetWidth() - w) / 2;
    float y = line_height * i;
    worker.Enqueue(std::bind(&Font::Print, &font, x, y, kMenuOption[i],
                             image->GetBuffer(), image->GetWidth()));
  }
  worker.Join();

  image->Compress();
  return image;
}

bool Menu::IsAnimating() {
  for (int i = 0; i < kOption_Max; ++i) {
    if (items_[i].text_animator.IsPlaying(Animator::kBlending))
      return true;
  }
  return false;
}
