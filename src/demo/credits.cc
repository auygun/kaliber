#include "credits.h"

#include "../base/log.h"
#include "../base/vecmath.h"
#include "../base/worker.h"
#include "../engine/engine.h"
#include "../engine/font.h"
#include "../engine/image.h"
#include "../engine/input_event.h"
#include "../engine/renderer/texture.h"
#include "demo.h"

using namespace base;
using namespace eng;

namespace {

constexpr char kCreditsLines[Credits::kNumLines][15] = {
    "Credits", "Code:", "Attila Uygun", "Graphics:", "Erkan Erturk"};

constexpr float kLineSpaces[Credits::kNumLines - 1] = {1.5f, 0.5f, 1.5f, 0.5f};

const Vector4 kTextColor = {0.3f, 0.55f, 1.0f, 1};
constexpr float kFadeSpeed = 0.2f;

}  // namespace

Credits::Credits() = default;

Credits::~Credits() = default;

bool Credits::Initialize() {
  Engine& engine = Engine::Get();

  font_ = engine.GetAsset<Font>("PixelCaps!.ttf");
  if (!font_)
    return false;

  max_text_width_ = -1;
  for (int i = 0; i < kNumLines; ++i) {
    int width, height;
    font_->CalculateBoundingBox(kCreditsLines[i], width, height);
    if (width > max_text_width_)
      max_text_width_ = width;
  }

  for (int i = 0; i < kNumLines; ++i)
    text_animator_.Attach(&text_[i]);

  return true;
}

void Credits::Update(float delta_time) {
  text_animator_.Update(delta_time);
}

void Credits::OnInputEvent(std::unique_ptr<InputEvent> event) {
  if ((event->GetType() == InputEvent::kTap ||
       event->GetType() == InputEvent::kDragEnd ||
       event->GetType() == InputEvent::kNavigateBack) &&
      !text_animator_.IsPlaying(Animator::kBlending)) {
    Hide();
    Engine& engine = Engine::Get();
    static_cast<Demo*>(engine.GetGame())->EnterMenuState();
  }
}

void Credits::Draw() {
  for (int i = 0; i < kNumLines; ++i)
    text_[i].Draw();
}

void Credits::ContextLost() {
  if (tex_)
    tex_->Update(CreateImage());
}

void Credits::Show() {
  tex_ = Engine::Get().CreateRenderResource<Texture>();
  tex_->Update(CreateImage());

  for (int i = 0; i < kNumLines; ++i) {
    text_[i].Create(tex_, {1, kNumLines});
    text_[i].SetOffset({0, 0});
    text_[i].SetScale({1, 1});
    text_[i].AutoScale();
    text_[i].SetColor(kTextColor * Vector4(1, 1, 1, 0));
    text_[i].SetFrame(i);

    if (i > 0) {
      text_[i].PlaceToBottomOf(text_[i - 1]);
      text_[i].Translate(text_[i - 1].GetOffset() * Vector2(0, 1));
      text_[i].Translate({0, text_[i - 1].GetScale().y * -kLineSpaces[i - 1]});
    }
  }

  float center_offset_y =
      (text_[0].GetOffset().y - text_[kNumLines - 1].GetOffset().y) / 2;
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
    tex_.reset();
    text_animator_.SetEndCallback(Animator::kBlending, nullptr);
    text_animator_.SetVisible(false);
  });
  text_animator_.SetBlending(kTextColor * Vector4(1, 1, 1, 0), kFadeSpeed);
  text_animator_.Play(Animator::kBlending, false);
}

std::shared_ptr<Image> Credits::CreateImage() {
  int line_height = font_->GetLineHeight() + 1;
  auto image = std::make_shared<Image>();
  image->Create(max_text_width_, line_height * kNumLines);
  image->Clear({1, 1, 1, 0});

  Worker worker(kNumLines);
  for (int i = 0; i < kNumLines; ++i) {
    int w, h;
    font_->CalculateBoundingBox(kCreditsLines[i], w, h);
    float x = (image->GetWidth() - w) / 2;
    float y = line_height * i;
    worker.Enqueue(std::bind(&Font::Print, font_, x, y, kCreditsLines[i],
                             image->GetBuffer(), image->GetWidth()));
  }
  worker.Join();

  image->SetImmutable();
  return image;
}
