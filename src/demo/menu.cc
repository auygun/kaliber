#include "demo/menu.h"

#include <cmath>
#include <string>
#include <vector>

#include "base/collusion_test.h"
#include "base/interpolation.h"
#include "base/log.h"
#include "engine/asset/font.h"
#include "engine/asset/image.h"
#include "engine/asset/sound.h"
#include "engine/engine.h"
#include "engine/input_event.h"
#include "engine/renderer/renderer.h"

#include "demo/demo.h"

using namespace std::string_literals;

using namespace base;
using namespace eng;

namespace {

constexpr char kVersionStr[] = "Version 1.0.4";

constexpr char kMenuOption[Menu::kOption_Max][10] = {"continue", "start",
                                                     "credits", "exit"};

constexpr float kMenuOptionSpace = 1.5f;

const Vector4f kColorNormal = {1, 1, 1, 1};
const Vector4f kColorHighlight = {20, 20, 20, 1};
constexpr float kBlendingSpeed = 0.12f;

const std::array<Vector4f, 2> kColorSwitch = {Vector4f{0.003f, 0.91f, 0.99f, 1},
                                              Vector4f{0.33f, 0.47, 0.51f, 1}};

const Vector4f kColorFadeIn = {1, 1, 1, 1};
const Vector4f kColorFadeOut = {1, 1, 1, 0};
constexpr float kFadeSpeed = 0.2f;

const Vector4f kHighScoreColor = {0.895f, 0.692f, 0.24f, 1};

const char kLastWave[] = "last_wave";

constexpr int kMaxStartWave = 10;

}  // namespace

Menu::Menu() = default;

Menu::~Menu() = default;

bool Menu::PreInitialize() {
  click_sound_ = std::make_shared<Sound>();
  if (!click_sound_->Load("demo/menu_click.mp3", false))
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

  Engine::Get().SetImageSource("menu_tex",
                               std::bind(&Menu::CreateMenuImage, this), true);
  Engine::Get().SetImageSource("logo_tex0",
                               "demo/woom_logo_start_frames_01.png", true);
  Engine::Get().SetImageSource("logo_tex1",
                               "demo/woom_logo_start_frames_02-03.png", true);
  Engine::Get().SetImageSource("buttons_tex", "demo/menu_icons.png", true);
  Engine::Get().SetImageSource("renderer_logo", "demo/renderer_logo.png", true);

  Engine::Get().SetImageSource(
      "version_tex",
      []() -> std::unique_ptr<Image> {
        const Font* font = Engine::Get().GetSystemFont();

        int w, h;
        font->CalculateBoundingBox(kVersionStr, w, h);

        auto image = std::make_unique<Image>();
        image->Create(w, font->GetLineHeight());
        image->Clear({1, 1, 1, 0});

        font->Print(0, 0, kVersionStr, image->GetBuffer(), image->GetWidth());

        image->Compress();
        return image;
      },
      true);

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

  return true;
}

bool Menu::Initialize() {
  Demo* game = static_cast<Demo*>(Engine::Get().GetGame());

  for (int i = 0; i < kOption_Max; ++i) {
    items_[i].text.Create("menu_tex", {1, 4});
    items_[i].text.SetZOrder(41);
    items_[i].text.Scale(1.5f);
    items_[i].text.SetColor(kColorFadeOut);
    items_[i].text.SetVisible(false);
    items_[i].text.SetFrame(i);

    items_[i].select_item_cb_ = [&, i]() -> void {
      items_[i].text_animator.SetEndCallback(Animator::kBlending, nullptr);
      // Wait until click sound ends before exiting.
      if (i == kExit)
        click_.SetEndCallback(
            [&, i]() -> void { selected_option_ = (Option)i; });
      else
        selected_option_ = (Option)i;
    };
    items_[i].text_animator.Attach(&items_[i].text);
  }
  // Get the item positions calculated.
  SetOptionEnabled(kContinue, true);

  click_.SetSound(click_sound_);
  click_.SetVariate(false);
  click_.SetSimulateStereo(false);
  click_.SetMaxAmplitude(1.5f);

  logo_[0].Create("logo_tex0", {3, 8});
  logo_[0].SetZOrder(41);
  logo_[0].SetPosition(Engine::Get().GetViewportSize() * Vector2f(0, 0.35f));

  logo_[1].Create("logo_tex1", {3, 7});
  logo_[1].SetZOrder(41);
  logo_[1].SetPosition(Engine::Get().GetViewportSize() * Vector2f(0, 0.35f));

  logo_animator_[0].Attach(&logo_[0]);
  logo_animator_[0].SetFrames(24, 20);
  logo_animator_[0].SetEndCallback(Animator::kFrames, [&]() -> void {
    logo_[0].SetVisible(false);
    logo_[1].SetVisible(true);
    logo_[1].SetFrame(0);
    logo_animator_[1].SetFrames(12, 20);
    logo_animator_[1].SetTimer(
        Lerp(3.0f, 8.0f, Engine::Get().GetRandomGenerator().Rand()));
    logo_animator_[1].Play(Animator::kFrames, true);
    logo_animator_[1].Play(Animator::kTimer, false);
  });

  logo_animator_[1].Attach(&logo_[1]);
  logo_animator_[1].SetEndCallback(Animator::kTimer, [&]() -> void {
    logo_animator_[1].Stop(Animator::kFrames);
    logo_[1].SetFrame(12);
    logo_animator_[1].SetFrames(9, 30);
    logo_animator_[1].SetTimer(
        Lerp(3.0f, 8.0f, Engine::Get().GetRandomGenerator().Rand()));
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
          else
            click_.Play(false);
        } else {
          game->SetEnableMusic(false);
        }
        game->saved_data().root()["audio"] = toggle_audio_.enabled();
      },
      true, game->saved_data().root().get("audio", Json::Value(true)).asBool(),
      kColorFadeOut, kColorSwitch);
  toggle_audio_.image().SetPosition(Engine::Get().GetViewportSize() *
                                    Vector2f(0, -0.25f));
  toggle_audio_.image().Scale(0.7f);

  toggle_music_.Create(
      "buttons_tex", {4, 2}, 1, 5,
      [&] {
        Demo* game = static_cast<Demo*>(Engine::Get().GetGame());
        game->SetEnableMusic(toggle_music_.enabled());
        game->saved_data().root()["music"] = toggle_music_.enabled();
      },
      true, game->saved_data().root().get("music", Json::Value(true)).asBool(),
      kColorFadeOut, kColorSwitch);
  toggle_music_.image().SetPosition(Engine::Get().GetViewportSize() *
                                    Vector2f(0, -0.25f));
  toggle_music_.image().Scale(0.7f);

  toggle_vibration_.Create(
      "buttons_tex", {4, 2}, 3, 7,
      [&] {
        Engine::Get().SetEnableVibration(toggle_vibration_.enabled());
        if (toggle_vibration_.enabled())
          Engine::Get().Vibrate(50);
        Demo* game = static_cast<Demo*>(Engine::Get().GetGame());
        game->saved_data().root()["vibration"] = toggle_vibration_.enabled();
      },
      true,
      game->saved_data().root().get("vibration", Json::Value(true)).asBool(),
      kColorFadeOut, kColorSwitch);
  toggle_vibration_.image().SetPosition(Engine::Get().GetViewportSize() *
                                        Vector2f(0, -0.25f));
  toggle_vibration_.image().Scale(0.7f);

  toggle_audio_.image().PlaceToLeftOf(toggle_music_.image());
  toggle_audio_.image().Translate({toggle_music_.image().GetSize().x / -2, 0});
  toggle_vibration_.image().PlaceToRightOf(toggle_music_.image());
  toggle_vibration_.image().Translate(
      {toggle_music_.image().GetSize().x / 2, 0});

  renderer_type_.Create(
      "renderer_logo", {2, 1}, 0, 1,
      [&] {
        Engine::Get().CreateRenderer(renderer_type_.enabled()
                                         ? RendererType::kVulkan
                                         : RendererType::kOpenGL);
      },
      true, Engine::Get().GetRendererType() == RendererType::kVulkan,
      kColorFadeOut, {Vector4f{1, 1, 1, 1}, Vector4f{1, 1, 1, 1}});
  renderer_type_.image().PlaceToBottomOf(toggle_music_.image());
  renderer_type_.image().Translate(toggle_music_.image().GetPosition() *
                                   Vector2f(0, 1.1f));

  high_score_value_ = game->GetHighScore();

  high_score_.Create("high_score_tex");
  high_score_.SetZOrder(41);
  high_score_.Scale(0.8f);
  high_score_.SetPosition(Engine::Get().GetViewportSize() *
                          Vector2f(0, 0.225f));
  high_score_.SetColor(kHighScoreColor * Vector4f(1, 1, 1, 0));
  high_score_.SetVisible(false);

  high_score_animator_.Attach(&high_score_);

  version_.Create("version_tex");
  version_.SetZOrder(41);
  version_.Scale(0.6f);
  version_.SetPosition(Engine::Get().GetViewportSize() * Vector2f(0, -0.5f) +
                       version_.GetSize() * Vector2f(0, 2));
  version_.SetColor(kHighScoreColor * Vector4f(1, 1, 1, 0));
  version_.SetVisible(false);

  version_animator_.Attach(&version_);

  start_from_wave_ = 1;
  starting_wave_.Create("starting_wave");

  wave_up_.Create(
      "wave_up_tex", {1, 1}, 0, 0,
      [&] {
        Demo* game = static_cast<Demo*>(Engine::Get().GetGame());
        start_from_wave_ += 3;
        if (start_from_wave_ > game->saved_data()
                                   .root()
                                   .get(kLastWave, Json::Value(1))
                                   .asInt() ||
            start_from_wave_ > kMaxStartWave)
          start_from_wave_ = 1;
        starting_wave_.image().SetFrame(start_from_wave_ / 3);
        click_.Play(false);
      },
      false, true, kColorFadeOut, kColorSwitch);
  wave_up_.image().Scale(1.5f);

  return true;
}

void Menu::OnInputEvent(std::unique_ptr<InputEvent> event) {
  if (toggle_audio_.OnInputEvent(event.get()) ||
      toggle_music_.OnInputEvent(event.get()) ||
      toggle_vibration_.OnInputEvent(event.get()) ||
      renderer_type_.OnInputEvent(event.get()) ||
      (wave_up_.image().IsVisible() && wave_up_.OnInputEvent(event.get())))
    return;

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

    click_.Play(false);
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

void Menu::SetRendererType() {
  renderer_type_.SetEnabled(
      (Engine::Get().GetRendererType() == RendererType::kVulkan));
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
      high_score_animator_.Play(Animator::kBlending, true);
      high_score_animator_.Play(Animator::kTimer, false);
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
  renderer_type_.Show();

  Demo* game = static_cast<Demo*>(Engine::Get().GetGame());

  if (!items_[kNewGame].hide &&
      game->saved_data().root().get(kLastWave, Json::Value(1)).asInt() > 3) {
    wave_up_.image().SetPosition(items_[1].text.GetPosition());
    wave_up_.image().PlaceToRightOf(items_[1].text);
    starting_wave_.image().SetPosition(wave_up_.image().GetPosition());
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
  renderer_type_.Hide();

  if (starting_wave_.image().IsVisible()) {
    starting_wave_.Hide();
    wave_up_.Hide();
  }
}

std::unique_ptr<Image> Menu::CreateMenuImage() {
  const Font& font = static_cast<Demo*>(Engine::Get().GetGame())->GetFont();

  int line_height = font.GetLineHeight() + 1;
  auto image = std::make_unique<Image>();
  image->Create(max_text_width_, line_height * kOption_Max);

  // Fill the area of each menu item with gradient.
  image->GradientV({0.80f, 0.87f, 0.93f, 0},
                   kColorSwitch[0] * Vector4f(1, 1, 1, 0), line_height);

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
                          bool enabled,
                          const Vector4f& fade_out_color,
                          const std::array<Vector4f, 2>& switch_color) {
  frame1_ = frame1;
  frame2_ = frame2;
  pressed_cb_ = std::move(pressed_cb);
  switch_control_ = switch_control;
  enabled_ = enabled;
  fade_out_color_ = fade_out_color;
  switch_color_ = switch_color;

  image_.Create(asset_name, num_frames);
  image_.SetFrame(enabled ? frame1 : frame2);
  image_.SetColor(fade_out_color_);
  image_.SetZOrder(41);
  image_.SetVisible(false);

  animator_.Attach(&image_);
}

bool Menu::Button::OnInputEvent(eng::InputEvent* event) {
  if (event->GetType() == InputEvent::kDragStart) {
    tap_pos_[0] = event->GetVector();
    tap_pos_[1] = event->GetVector();
  } else if (event->GetType() == InputEvent::kDrag) {
    tap_pos_[1] = event->GetVector();
  }

  if (event->GetType() != InputEvent::kDragEnd)
    return false;

  if (!Intersection(image_.GetPosition(), image_.GetSize() * Vector2f(1.2f, 2),
                    tap_pos_[0]))
    return false;
  if (!Intersection(image_.GetPosition(), image_.GetSize() * Vector2f(1.2f, 2),
                    tap_pos_[1]))
    return false;

  SetEnabled(!enabled_);
  pressed_cb_();

  return true;
}

void Menu::Button::Show() {
  animator_.SetVisible(true);
  animator_.SetBlending(enabled_ ? switch_color_[0] : switch_color_[1],
                        kBlendingSpeed);
  animator_.Play(Animator::kBlending, false);
  animator_.SetEndCallback(Animator::kBlending, nullptr);
}

void Menu::Button::Hide() {
  animator_.SetBlending(fade_out_color_, kBlendingSpeed);
  animator_.Play(Animator::kBlending, false);
  animator_.SetEndCallback(Animator::kBlending,
                           [&]() -> void { animator_.SetVisible(false); });
}

void Menu::Button::SetEnabled(bool enable) {
  if (switch_control_) {
    enabled_ = enable;
    image_.SetFrame(enabled_ ? frame1_ : frame2_);
    image_.SetColor(enabled_ ? switch_color_[0] : switch_color_[1]);
  }
}

//
// Menu::Radio implementation
//

void Menu::Radio::Create(const std::string& asset_name) {
  Engine::Get().SetImageSource(asset_name,
                               std::bind(&Radio::CreateImage, this));

  options_.Create(asset_name, {1, (kMaxStartWave + 2) / 3});
  options_.SetZOrder(41);
  options_.SetColor(kColorFadeOut);
  options_.SetFrame(0);
  options_.SetVisible(false);

  animator_.Attach(&options_);
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
  for (int i = 1; i <= kMaxStartWave; i += 3) {
    int w, h;
    font.CalculateBoundingBox(std::to_string(i), w, h);
    if (w > max_width)
      max_width = w;
  }

  int line_height = font.GetLineHeight() + 1;

  auto image = std::make_unique<Image>();
  image->Create(max_width, line_height * ((kMaxStartWave + 2) / 3));
  image->Clear({1, 1, 1, 0});

  for (int i = 1, j = 0; i <= kMaxStartWave; i += 3) {
    int w, h;
    font.CalculateBoundingBox(std::to_string(i), w, h);
    float x = (image->GetWidth() - w) / 2;
    float y = line_height * j++;
    font.Print(x, y, std::to_string(i), image->GetBuffer(), image->GetWidth());
  }

  image->Compress();

  return image;
}
