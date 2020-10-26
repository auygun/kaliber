#include "menu.h"

#include <cmath>
#include <string>
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

using namespace std::string_literals;

using namespace base;
using namespace eng;

namespace {

constexpr char kVersionStr[] = "Version 1.0.test.6";

constexpr char kMenuOption[Menu::kOption_Max][10] = {"continue", "start",
                                                     "credits", "exit"};

constexpr float kMenuOptionSpace = 1.5f;

const Vector4 kColorNormal = {1, 1, 1, 1};
const Vector4 kColorHighlight = {10, 10, 10, 1};
constexpr float kBlendingSpeed = 0.12f;

const Vector4 kColorSwitch[2] = {{0.003f, 0.91f, 0.99f, 1},
                                 {0.33f, 0.47, 0.51f, 1}};

const Vector4 kColorFadeOut = {1, 1, 1, 0};
constexpr float kFadeSpeed = 0.2f;

const Vector4 kHighScoreColor = {0.895f, 0.692f, 0.24f, 1};

const char kLastWave[] = "last_wave";

}  // namespace

Menu::Menu() = default;

Menu::~Menu() = default;

bool Menu::Initialize() {
  click_sound_ = std::make_shared<Sound>();
  if (!click_sound_->Load("menu_click.mp3", false))
    return false;

  Demo* game = static_cast<Demo*>(Engine::Get().GetGame());

  const Font& font = game->GetFont();

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
    items_[i].text.SetZOrder(41);
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
  logo_[0].SetZOrder(41);
  logo_[0].SetOffset(Engine::Get().GetScreenSize() * Vector2(0, 0.35f));

  logo_[1].Create("logo_tex1", {3, 7});
  logo_[1].SetZOrder(41);
  logo_[1].SetOffset(Engine::Get().GetScreenSize() * Vector2(0, 0.35f));

  logo_animator_[0].Attach(&logo_[0]);
  logo_animator_[0].SetFrames(24, 20);
  logo_animator_[0].SetEndCallback(Animator::kFrames, [&]() -> void {
    logo_[0].SetVisible(false);
    logo_[1].SetVisible(true);
    logo_[1].SetFrame(0);
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

        Demo* game = static_cast<Demo*>(Engine::Get().GetGame());
        if (toggle_audio_.enabled()) {
          if (toggle_music_.enabled())
            game->SetEnableMusic(true);
        } else {
          game->SetEnableMusic(false);
        }
        game->saved_data()["audio"] << toggle_audio_.enabled();
      },
      true, game->saved_data().Get<bool>("audio", true));
  toggle_audio_.image().SetOffset(Engine::Get().GetScreenSize() *
                                  Vector2(0, -0.25f));
  toggle_audio_.image().Scale(0.7f);

  toggle_music_.Create(
      "buttons_tex", {4, 2}, 1, 5,
      [&] {
        Demo* game = static_cast<Demo*>(Engine::Get().GetGame());
        game->SetEnableMusic(toggle_music_.enabled());
        game->saved_data()["music"] << toggle_music_.enabled();
      },
      true, game->saved_data().Get<bool>("music", true));
  toggle_music_.image().SetOffset(Engine::Get().GetScreenSize() *
                                  Vector2(0, -0.25f));
  toggle_music_.image().Scale(0.7f);

  toggle_vibration_.Create(
      "buttons_tex", {4, 2}, 3, 7,
      [&] {
        Engine::Get().SetEnableVibration(toggle_vibration_.enabled());
        if (toggle_vibration_.enabled())
          Engine::Get().Vibrate(50);
        Demo* game = static_cast<Demo*>(Engine::Get().GetGame());
        game->saved_data()["vibration"] << toggle_vibration_.enabled();
      },
      true, game->saved_data().Get<bool>("vibration", true));
  toggle_vibration_.image().SetOffset(Engine::Get().GetScreenSize() *
                                      Vector2(0, -0.25f));
  toggle_vibration_.image().Scale(0.7f);

  toggle_audio_.image().PlaceToLeftOf(toggle_music_.image());
  toggle_audio_.image().Translate({toggle_music_.image().GetScale().x / -2, 0});
  toggle_vibration_.image().PlaceToRightOf(toggle_music_.image());
  toggle_vibration_.image().Translate(
      {toggle_music_.image().GetScale().x / 2, 0});

  high_score_value_ = game->GetHighScore();

  high_score_.Create("high_score_tex");
  high_score_.SetZOrder(41);
  high_score_.Scale(0.8f);
  high_score_.SetOffset(Engine::Get().GetScreenSize() * Vector2(0, 0.225f));
  high_score_.SetColor(kHighScoreColor * Vector4(1, 1, 1, 0));
  high_score_.SetVisible(false);

  high_score_animator_.Attach(&high_score_);

  version_.Create("version_tex");
  version_.SetZOrder(41);
  version_.Scale(0.6f);
  version_.SetOffset(Engine::Get().GetScreenSize() * Vector2(0, -0.5f) +
                     version_.GetScale() * Vector2(0, 2));
  version_.SetColor(kHighScoreColor * Vector4(1, 1, 1, 0));
  version_.SetVisible(false);

  version_animator_.Attach(&version_);

  start_from_wave_ = 1;
  starting_wave_.Create("starting_wave");

  wave_up_.Create(
      "wave_up_tex", {1, 1}, 0, 0,
      [&] {
        Demo* game = static_cast<Demo*>(Engine::Get().GetGame());
        start_from_wave_ += 3;
        if (start_from_wave_ > game->saved_data().Get<int>(kLastWave, 1) ||
            start_from_wave_ > 10)
          start_from_wave_ = 1;
        starting_wave_.image().SetFrame(start_from_wave_ / 3);
      },
      false, true);
  wave_up_.image().Scale(1.5f);

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

  high_score_animator_.Update(delta_time);

  version_animator_.Update(delta_time);

  starting_wave_.Update(delta_time);
  wave_up_.Update(delta_time);
}

void Menu::OnInputEvent(std::unique_ptr<InputEvent> event) {
  if (toggle_audio_.OnInputEvent(event.get()) ||
      toggle_music_.OnInputEvent(event.get()) ||
      toggle_vibration_.OnInputEvent(event.get()) ||
      (wave_up_.image().IsVisible() && wave_up_.OnInputEvent(event.get())))
    return;

  if (event->GetType() == InputEvent::kDragStart)
    tap_pos_[0] = tap_pos_[1] = event->GetVector();
  else if (event->GetType() == InputEvent::kDrag)
    tap_pos_[1] = event->GetVector();

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

  if (high_score_value_ !=
      static_cast<Demo*>(Engine::Get().GetGame())->GetHighScore()) {
    high_score_value_ =
        static_cast<Demo*>(Engine::Get().GetGame())->GetHighScore();
    Engine::Get().RefreshImage("high_score_tex");

    high_score_animator_.SetEndCallback(Animator::kBlending, [&]() -> void {
      high_score_animator_.SetBlending(kColorFadeOut, 0.3f);
      high_score_animator_.SetTimer(5);
      high_score_animator_.Play(Animator::kBlending | Animator::kTimer, true);
    });
    high_score_animator_.SetEndCallback(Animator::kTimer, [&]() -> void {
      high_score_animator_.Play(Animator::kBlending | Animator::kTimer, false);
      high_score_animator_.SetEndCallback(Animator::kBlending, [&]() -> void {
        high_score_animator_.Stop(Animator::kBlending);
      });
      high_score_animator_.SetEndCallback(Animator::kTimer, nullptr);
    });
  }
  if (high_score_value_ > 0) {
    high_score_animator_.SetVisible(true);
    high_score_animator_.SetBlending(kHighScoreColor, kFadeSpeed);
    high_score_animator_.Play(Animator::kBlending, false);
  }

  version_animator_.SetVisible(true);
  version_animator_.SetBlending(kHighScoreColor, kFadeSpeed);
  version_animator_.Play(Animator::kBlending, false);

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

  Demo* game = static_cast<Demo*>(Engine::Get().GetGame());

  if (!items_[kNewGame].hide && game->saved_data().Get<int>(kLastWave, 1) > 3) {
    wave_up_.image().SetOffset(items_[1].text.GetOffset());
    wave_up_.image().PlaceToRightOf(items_[1].text);
    starting_wave_.image().SetOffset(wave_up_.image().GetOffset());
    starting_wave_.Show();
    wave_up_.Show();
  }
}

void Menu::Hide(Closure cb) {
  for (int i = 0; i < 2; ++i) {
    logo_animator_[i].Stop(Animator::kAllAnimations | Animator::kTimer);
    logo_animator_[i].SetBlending(kColorFadeOut, kFadeSpeed);
    logo_animator_[i].SetEndCallback(Animator::kBlending, [&, i, cb]() -> void {
      logo_animator_[i].Stop(Animator::kAllAnimations | Animator::kTimer);
      logo_animator_[i].SetEndCallback(Animator::kBlending, nullptr);
      logo_animator_[i].SetVisible(false);
      if (i == 0 && cb)
        cb();
    });
    logo_animator_[i].Play(Animator::kBlending, false);
  }

  high_score_animator_.Stop(Animator::kAllAnimations | Animator::kTimer);
  high_score_animator_.SetEndCallback(Animator::kTimer, nullptr);
  high_score_animator_.SetBlending(kColorFadeOut, kFadeSpeed);
  high_score_animator_.SetEndCallback(Animator::kBlending, [&]() -> void {
    high_score_animator_.SetEndCallback(Animator::kBlending, nullptr);
    high_score_animator_.SetVisible(false);
  });
  high_score_animator_.Play(Animator::kBlending, false);

  version_animator_.Stop(Animator::kAllAnimations);
  version_animator_.SetBlending(kColorFadeOut, kFadeSpeed);
  version_animator_.SetEndCallback(Animator::kBlending, [&]() -> void {
    version_animator_.SetEndCallback(Animator::kBlending, nullptr);
    version_animator_.SetVisible(false);
  });
  version_animator_.Play(Animator::kBlending, false);

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

  if (starting_wave_.image().IsVisible()) {
    starting_wave_.Hide();
    wave_up_.Hide();
  }
}

bool Menu::CreateRenderResources() {
  Engine::Get().SetImageSource("menu_tex",
                               std::bind(&Menu::CreateMenuImage, this));
  Engine::Get().SetImageSource("logo_tex0", "woom_logo_start_frames_01.png");
  Engine::Get().SetImageSource("logo_tex1", "woom_logo_start_frames_02-03.png");
  Engine::Get().SetImageSource("buttons_tex", "menu_icons.png");
  Engine::Get().SetImageSource("high_score_tex",
                               std::bind(&Menu::CreateHighScoreImage, this));

  Engine::Get().SetImageSource("wave_up_tex", []() -> std::unique_ptr<Image> {
    const Font& font = static_cast<Demo*>(Engine::Get().GetGame())->GetFont();

    constexpr char btn_text[] = "[  ]";

    int w, h;
    font.CalculateBoundingBox(btn_text, w, h);

    auto image = std::make_unique<Image>();
    image->Create(w, h);
    image->Clear({1, 1, 1, 0});

    font.Print(0, 0, btn_text, image->GetBuffer(), image->GetWidth());

    image->Compress();
    return image;
  });

  Engine::Get().SetImageSource("version_tex", []() -> std::unique_ptr<Image> {
    const Font* font = Engine::Get().GetSystemFont();

    int w, h;
    font->CalculateBoundingBox(kVersionStr, w, h);

    auto image = std::make_unique<Image>();
    image->Create(w, font->GetLineHeight());
    image->Clear({1, 1, 1, 0});

    font->Print(0, 0, kVersionStr, image->GetBuffer(), image->GetWidth());

    image->Compress();
    return image;
  });

  return true;
}

std::unique_ptr<Image> Menu::CreateMenuImage() {
  const Font& font = static_cast<Demo*>(Engine::Get().GetGame())->GetFont();

  int line_height = font.GetLineHeight() + 1;
  auto image = std::make_unique<Image>();
  image->Create(max_text_width_, line_height * kOption_Max);

  // Fill the area of each menu item with gradient.
  image->GradientV({0.80f, 0.87f, 0.93f, 0},
                   kColorSwitch[0] * Vector4(1, 1, 1, 0), line_height);

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

std::unique_ptr<Image> Menu::CreateHighScoreImage() {
  std::string text = "High Score: "s + std::to_string(high_score_value_);
  const Font& font = static_cast<Demo*>(Engine::Get().GetGame())->GetFont();

  int width, height;
  font.CalculateBoundingBox(text, width, height);

  auto image = std::make_unique<Image>();
  image->Create(width, height);
  image->Clear({1, 1, 1, 0});
  font.Print(0, 0, text, image->GetBuffer(), image->GetWidth());

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

//
// Menu::Button implementation
//

void Menu::Button::Create(const std::string& asset_name,
                          std::array<int, 2> num_frames,
                          int frame1,
                          int frame2,
                          Closure pressed_cb,
                          bool switch_control,
                          bool enabled) {
  frame1_ = frame1;
  frame2_ = frame2;
  pressed_cb_ = std::move(pressed_cb);
  switch_control_ = switch_control;
  enabled_ = enabled;

  image_.Create(asset_name, num_frames);
  image_.SetFrame(enabled ? frame1 : frame2);
  image_.SetColor(kColorFadeOut);
  image_.SetZOrder(41);
  image_.SetVisible(false);

  animator_.Attach(&image_);
}

void Menu::Button::Update(float delta_time) {
  animator_.Update(delta_time);
}

bool Menu::Button::OnInputEvent(eng::InputEvent* event) {
  if (event->GetType() == InputEvent::kDragStart)
    tap_pos_[0] = tap_pos_[1] = event->GetVector();
  else if (event->GetType() == InputEvent::kDrag)
    tap_pos_[1] = event->GetVector();

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

void Menu::Button::Show() {
  animator_.SetVisible(true);
  animator_.SetBlending(enabled_ ? kColorSwitch[0] : kColorSwitch[1],
                        kBlendingSpeed);
  animator_.Play(Animator::kBlending, false);
  animator_.SetEndCallback(Animator::kBlending, nullptr);
}

void Menu::Button::Hide() {
  animator_.SetBlending(kColorFadeOut, kBlendingSpeed);
  animator_.Play(Animator::kBlending, false);
  animator_.SetEndCallback(Animator::kBlending,
                           [&]() -> void { animator_.SetVisible(false); });
}

void Menu::Button::SetEnabled(bool enable) {
  if (switch_control_) {
    enabled_ = enable;
    image_.SetFrame(enabled_ ? frame1_ : frame2_);
    image_.SetColor(enabled_ ? kColorSwitch[0] : kColorSwitch[1]);
  }
}

//
// Menu::Radio implementation
//

void Menu::Radio::Create(const std::string& asset_name) {
  Engine::Get().SetImageSource(asset_name,
                               std::bind(&Radio::CreateImage, this));

  options_.Create(asset_name, {1, 4});
  options_.SetZOrder(41);
  options_.SetColor(kColorFadeOut);
  options_.SetFrame(0);
  options_.SetVisible(false);

  animator_.Attach(&options_);
}

void Menu::Radio::Update(float delta_time) {
  animator_.Update(delta_time);
}

bool Menu::Radio::OnInputEvent(eng::InputEvent* event) {
  return false;
}

void Menu::Radio::Show() {
  animator_.SetVisible(true);
  animator_.SetBlending(kHighScoreColor, kBlendingSpeed);
  animator_.Play(Animator::kBlending, false);
  animator_.SetEndCallback(Animator::kBlending, nullptr);
}

void Menu::Radio::Hide() {
  animator_.SetBlending(kColorFadeOut, kBlendingSpeed);
  animator_.Play(Animator::kBlending, false);
  animator_.SetEndCallback(Animator::kBlending,
                           [&]() -> void { animator_.SetVisible(false); });
}

std::unique_ptr<eng::Image> Menu::Radio::CreateImage() {
  const Font& font = static_cast<Demo*>(Engine::Get().GetGame())->GetFont();

  int max_width = 0;
  for (int i = 1; i <= 10; i += 3) {
    int w, h;
    font.CalculateBoundingBox(std::to_string(i), w, h);
    if (w > max_width)
      max_width = w;
  }

  int line_height = font.GetLineHeight() + 1;

  auto image = std::make_unique<Image>();
  image->Create(max_width, line_height * 4);
  image->Clear({1, 1, 1, 0});

  for (int i = 1, j = 0; i <= 10; i += 3) {
    int w, h;
    font.CalculateBoundingBox(std::to_string(i), w, h);
    float x = (image->GetWidth() - w) / 2;
    float y = line_height * j++;
    font.Print(x, y, std::to_string(i), image->GetBuffer(), image->GetWidth());
  }

  image->Compress();

  return image;
}
