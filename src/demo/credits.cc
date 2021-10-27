#include "demo/credits.h"

#include "base/log.h"
#include "base/vecmath.h"
#include "demo/demo.h"
#include "engine/engine.h"
#include "engine/font.h"
#include "engine/image.h"
#include "engine/input_event.h"

using namespace base;
using namespace eng;

namespace {

constexpr char kCreditsLines[Credits::kNumLines][40] = {
    "Credits",          "Code",
    "Attila Uygun",     "Graphics",
    "Erkan Ertürk",     "Music",
    "Patrik Häggblad",  "Special thanks",
    "Peter Pettersson", "github.com/auygun/kaliber"};

constexpr float kLineSpaces[Credits::kNumLines - 1] = {
    1.5f, 0.5f, 1.5f, 0.5f, 1.5f, 0.5f, 1.5f, 0.5f, 1.5f};

const Vector4f kTextColor = {0.80f, 0.87f, 0.93f, 1};
constexpr float kFadeSpeed = 0.2f;

}  // namespace

Credits::Credits() = default;

Credits::~Credits() = default;

bool Credits::Initialize() {
  const Font* font = Engine::Get().GetSystemFont();

  max_text_width_ = -1;
  for (int i = 0; i < kNumLines; ++i) {
    int width, height;
    font->CalculateBoundingBox(kCreditsLines[i], width, height);
    if (width > max_text_width_)
      max_text_width_ = width;
  }

  Engine::Get().SetImageSource("credits",
                               std::bind(&Credits::CreateImage, this));

  for (int i = 0; i < kNumLines; ++i)
    text_animator_.Attach(&text_[i]);

  return true;
}

void Credits::OnInputEvent(std::unique_ptr<InputEvent> event) {
  if ((event->GetType() == InputEvent::kDragEnd ||
       event->GetType() == InputEvent::kNavigateBack) &&
      !text_animator_.IsPlaying(Animator::kBlending)) {
    Hide();
    Engine& engine = Engine::Get();
    static_cast<Demo*>(engine.GetGame())->EnterMenuState();
  }
}

void Credits::Show() {
  Engine::Get().RefreshImage("credits");

  for (int i = 0; i < kNumLines; ++i) {
    text_[i].Create("credits", {1, kNumLines});
    text_[i].SetZOrder(50);
    text_[i].SetPosition({0, 0});
    text_[i].SetColor(kTextColor * Vector4f(1, 1, 1, 0));
    text_[i].SetFrame(i);

    if (i > 0) {
      text_[i].PlaceToBottomOf(text_[i - 1]);
      text_[i].Translate(text_[i - 1].GetPosition() * Vector2f(0, 1));
      text_[i].Translate({0, text_[i - 1].GetSize().y * -kLineSpaces[i - 1]});
    }
  }

  float center_offset_y =
      (text_[0].GetPosition().y - text_[kNumLines - 1].GetPosition().y) / 2;
  for (int i = 0; i < kNumLines; ++i)
    text_[i].Translate({0, center_offset_y});

  text_animator_.SetEndCallback(Animator::kBlending, [&]() -> void {
    text_animator_.SetEndCallback(Animator::kBlending, nullptr);
  });
  text_animator_.SetBlending(kTextColor, kFadeSpeed);
  text_animator_.Play(Animator::kBlending, false);
  text_animator_.SetVisible(true);
}

void Credits::Hide() {
  text_animator_.SetEndCallback(Animator::kBlending, [&]() -> void {
    for (int i = 0; i < kNumLines; ++i)
      text_[i].Destory();
    text_animator_.SetEndCallback(Animator::kBlending, nullptr);
    text_animator_.SetVisible(false);
  });
  text_animator_.SetBlending(kTextColor * Vector4f(1, 1, 1, 0), kFadeSpeed);
  text_animator_.Play(Animator::kBlending, false);
}

std::unique_ptr<Image> Credits::CreateImage() {
  const Font* font = Engine::Get().GetSystemFont();

  int line_height = font->GetLineHeight() + 1;
  auto image = std::make_unique<Image>();
  image->Create(max_text_width_, line_height * kNumLines);
  image->Clear({1, 1, 1, 0});

  for (int i = 0; i < kNumLines; ++i) {
    int w, h;
    font->CalculateBoundingBox(kCreditsLines[i], w, h);
    float x = (image->GetWidth() - w) / 2;
    float y = line_height * i;
    font->Print(x, y, kCreditsLines[i], image->GetBuffer(), image->GetWidth());
  }

  image->Compress();
  return image;
}
