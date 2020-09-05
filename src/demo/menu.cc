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
#include "../engine/sound.h"
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

const Vector4 kColorSwitch[2] = {{0.003f, 0.91f, 0.99f, 1},
                                 {0.33f, 0.47, 0.51f, 1}};

const Vector4 kColorFadeOut = {1, 1, 1, 0};
constexpr float kFadeSpeed = 0.2f;

}  // namespace

void Switch::Create(const std::string& asset_name,
                    std::array<int, 2> num_frames,
                    int frame1,
                    int frame2,
                    Closure pressed_cb,
                    bool enabled) {
  frame1_ = frame1;
  frame2_ = frame2;
  pressed_cb_ = std::move(pressed_cb);
  enabled_ = enabled;

  image_.Create(asset_name, num_frames);
  image_.SetFrame(enabled ? frame1 : frame2);
  image_.SetColor(kColorFadeOut);
  image_.SetZOrder(41);
  image_.Scale(0.7f);
  image_.SetVisible(false);

  animator_.Attach(&image_);
}

void Switch::Update(float delta_time) {
  animator_.Update(delta_time);
}

bool Switch::OnInputEvent(eng::InputEvent* event) {
  if (event->GetType() == InputEvent::kDragStart)
    tap_pos_[0] = tap_pos_[1] = event->GetVector(0);
  else if (event->GetType() == InputEvent::kDrag)
    tap_pos_[1] = event->GetVector(0);

  if (event->GetType() != InputEvent::kDragEnd)
    return false;

  if (!Intersection(image_.GetOffset(), image_.GetScale() * Vector2(1.2f, 2),
                    tap_pos_[0]))
    return false;
  if (!Intersection(image_.GetOffset(), image_.GetScale() * Vector2(1.2f, 2),
                    tap_pos_[1]))
    return false;

  SetEnabled(!enabled_);
  pressed_cb_();

  return true;
}

void Switch::Show() {
  animator_.SetVisible(true);
  animator_.SetBlending(enabled_ ? kColorSwitch[0] : kColorSwitch[1],
                        kBlendingSpeed);
  animator_.Play(Animator::kBlending, false);
  animator_.SetEndCallback(Animator::kBlending, nullptr);
}

void Switch::Hide() {
  animator_.SetBlending(kColorFadeOut, kBlendingSpeed);
  animator_.Play(Animator::kBlending, false);
  animator_.SetEndCallback(Animator::kBlending,
                           [&]() -> void { animator_.SetVisible(false); });
}

void Switch::SetEnabled(bool enable) {
  enabled_ = enable;
  image_.SetFrame(enabled_ ? frame1_ : frame2_);
  image_.SetColor(enabled_ ? kColorSwitch[0] : kColorSwitch[1]);
}

Menu::Menu() = default;

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

  click_.SetSound(click_sound_);
  click_.SetVariate(false);
  click_.SetSimulateStereo(false);
  click_.SetMaxAplitude(1.5f);

  logo_[0].Create("logo_tex0", {3, 8});
  logo_[0].SetZOrder(40);
  logo_[0].SetOffset(Engine::Get().GetScreenSize() * Vector2(0, 0.25f));

  logo_[1].Create("logo_tex1", {3, 7});
  logo_[1].SetZOrder(40);
  logo_[1].SetOffset(Engine::Get().GetScreenSize() * Vector2(0, 0.25f));

  logo_animator_[0].Attach(&logo_[0]);
  logo_animator_[0].SetFrames(24, 20);
  logo_animator_[0].SetEndCallback(Animator::kFrames, [&]() -> void {
    logo_[0].SetVisible(false);
    logo_[1].SetVisible(true);
    logo_animator_[1].SetFrames(12, 20);
    logo_animator_[1].SetTimer(
        Lerp(3.0f, 8.0f, Engine::Get().GetRandomGenerator().GetFloat()));
    logo_animator_[1].Play(Animator::kFrames | Animator::kTimer, true);
  });

  logo_animator_[1].Attach(&logo_[1]);
  logo_animator_[1].SetEndCallback(Animator::kTimer, [&]() -> void {
    logo_animator_[1].Stop(Animator::kFrames);
    logo_[1].SetFrame(12);
    logo_animator_[1].SetFrames(9, 30);
    logo_animator_[1].SetTimer(
        Lerp(3.0f, 8.0f, Engine::Get().GetRandomGenerator().GetFloat()));
    logo_animator_[1].Play(Animator::kFrames | Animator::kTimer, false);
  });
  logo_animator_[1].SetEndCallback(Animator::kFrames, [&]() -> void {
    logo_[1].SetFrame(0);
    logo_animator_[1].SetFrames(12, 20);
    logo_animator_[1].Play(Animator::kFrames, true);
  });

  toggle_audio_.Create(
      "buttons_tex", {4, 2}, 2, 6,
      [&] {
        Engine::Get().SetEnableAudio(toggle_audio_.enabled());
        if (toggle_audio_.enabled()) {
          if (toggle_music_.enabled())
            static_cast<Demo*>(Engine::Get().GetGame())->SetEnableMusic(true);
        } else {
          static_cast<Demo*>(Engine::Get().GetGame())->SetEnableMusic(false);
        }
      },
      true);
  toggle_audio_.image().SetOffset(Engine::Get().GetScreenSize() *
                                  Vector2(0, -0.25f));

  toggle_music_.Create(
      "buttons_tex", {4, 2}, 1, 5,
      [&] {
        static_cast<Demo*>(Engine::Get().GetGame())
            ->SetEnableMusic(toggle_music_.enabled());
      },
      true);
  toggle_music_.image().SetOffset(Engine::Get().GetScreenSize() *
                                  Vector2(0, -0.25f));

  toggle_vibration_.Create(
      "buttons_tex", {4, 2}, 3, 7,
      [&] {
        Engine::Get().SetEnableVibration(toggle_vibration_.enabled());
        if (toggle_vibration_.enabled())
          Engine::Get().Vibrate(50);
      },
      true);
  toggle_vibration_.image().SetOffset(Engine::Get().GetScreenSize() *
                                      Vector2(0, -0.25f));

  toggle_audio_.image().PlaceToLeftOf(toggle_music_.image());
  toggle_audio_.image().Translate({toggle_music_.image().GetScale().x / -2, 0});
  toggle_vibration_.image().PlaceToRightOf(toggle_music_.image());
  toggle_vibration_.image().Translate(
      {toggle_music_.image().GetScale().x / 2, 0});

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

  toggle_audio_.Update(delta_time);
  toggle_music_.Update(delta_time);
  toggle_vibration_.Update(delta_time);
}

void Menu::OnInputEvent(std::unique_ptr<InputEvent> event) {
  if (toggle_audio_.OnInputEvent(event.get()) ||
      toggle_music_.OnInputEvent(event.get()) ||
      toggle_vibration_.OnInputEvent(event.get()))
    return;

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

  toggle_audio_.Show();
  toggle_music_.Show();
  toggle_vibration_.Show();
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

  toggle_audio_.Hide();
  toggle_music_.Hide();
  toggle_vibration_.Hide();
}

bool Menu::CreateRenderResources() {
  Engine::Get().SetImageSource("menu_tex", std::bind(&Menu::CreateImage, this));
  Engine::Get().SetImageSource("logo_tex0", "woom_logo_start_frames_01.png");
  Engine::Get().SetImageSource("logo_tex1", "woom_logo_start_frames_02-03.png");
  Engine::Get().SetImageSource("buttons_tex", "menu_icons.png");

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
