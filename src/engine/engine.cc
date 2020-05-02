#include "engine.h"
#include "../base/log.h"
#include "../base/random.h"
#include "asset_manager/image.h"
#include "game.h"
#include "game_factory.h"
#include "drawable.h"
#include "input_event.h"
#include <algorithm>

namespace engine {

Engine& Engine::Get() {
  static Engine engine;
  return engine;
}

bool Engine::Init() {
  RandomInit();

  if (!font_.Create("Roboto-Regular.ttf")) {
    LOG << "Failed to create the font.";
    return false;
  }

  if (!CreateRenderResources())
    return false;

  game_ = engine::GameFactoryBase::CreateGame("");
  if (!game_) {
    printf("No game found to run.\n");
    return false;
  }

  if (!game_->Initialize()) {
    LOG << "Failed to initialize the game.";
    return false;
  }

  stats_.SetVisible(true);
  AddDrawable(&stats_);

  return true;
}

void Engine::Shutdown() {
  game_.reset();
}

void Engine::AddDrawable(Drawable* drawable) {
  assert(std::find(drawables_.begin(), drawables_.end(), drawable) ==
      drawables_.end());
  drawables_.push_back(drawable);
}

void Engine::RemoveDrawable(Drawable* drawable) {
  auto it = std::find(drawables_.begin(), drawables_.end(), drawable);
  if (it != drawables_.end()) {
    drawables_.erase(it);
    return;
  }
}

void Engine::Update(float delta_time) {
  seconds_accumulated_ += delta_time;
  game_->Update(delta_time);
  PrintStats();
}

void Engine::Draw(float frame_frac) {
  renderer_.EnterDrawStage();
  Clear();
  renderer_.EnableBlend();
  for (auto d : drawables_) {
    if (d->visible())
      d->Draw();
  }
  Present();
  renderer_.ExitDrawStage();
}

void Engine::Clear() {
  // Pulsate the clear color to make it more visible if we see it.
  static float grey = 0.0f;
  // grey += 0.01f;
  // if (grey > 1.0f)
  //   grey = 0.0f;
  renderer_.Clear({grey, grey, grey, 1.0f});
}

void Engine::Present() {
  renderer_.Present();
}

void Engine::TrimMemory() {
  renderer_.TrimMemory();
}

// TODO: do once during initialization.
Vector2 Engine::GetScreenSize() {
  if (GetRenderer().GetScreenWidth() > GetRenderer().GetScreenHeight()) {
    float ratio = (float)GetRenderer().GetScreenWidth() / (float)GetRenderer().GetScreenHeight();
    return Vector2(ratio * 2.0f, 2.0f);
  } else {
    float ratio = (float)GetRenderer().GetScreenHeight() / (float)GetRenderer().GetScreenWidth();
    return Vector2(2.0f, ratio * 2.0f);
  }
}

Vector2 Engine::ToScale(const Vector2& vec) {
  return GetScreenSize() * vec /
      Vector2((float)GetRenderer().GetScreenWidth(),
              (float)GetRenderer().GetScreenHeight());
}

Vector2 Engine::ToPosition(const Vector2& vec) {
  return ToScale(vec) - GetScreenSize() / 2.0f;
}

void Engine::AddInputEvent(std::unique_ptr<InputEvent> event) {
  input_queue_.push_back(std::move(event));
}

std::unique_ptr<InputEvent> Engine::GetNextInputEvent() {
  std::unique_ptr<InputEvent> event;
  if (!input_queue_.empty()) {
    event.swap(input_queue_.front());
    input_queue_.pop_front();
  }
  return event;
}

bool Engine::CreateRenderResources() {
  // Create the shader we can reuse for all tiles.
  const char* vertex_description = "p2f;t2f";
  if (!pass_through_shader_.Create("shaders/pass_through",
                                   vertex_description)) {
    LOG << "Could not create pass through shader.";
    return false;
  }

  // Create the quad geometry we can reuse for all sprites.
  // This creates a normalized unit sized quad.
  static const float vertices[] = {
    -0.5f, -0.5f, 0.0f, 1.0f,
     0.5f, -0.5f, 1.0f, 1.0f,
    -0.5f,  0.5f, 0.0f, 0.0f,
     0.5f,  0.5f, 1.0f, 0.0f
  };
  if (!quad_.Create(GL_TRIANGLE_STRIP, vertex_description, 4, vertices)) {
    LOG << "Could not create quad geometry.";
    return false;
  }

  return true;
}

void Engine::PrintStats() {
  constexpr int width = 300;
  std::vector<std::string> lines;
  std::string line = "frames dropped: ";
  line += std::to_string(engine::Engine::Get().GetRenderer().num_frames_dropped());
  lines.push_back(line);
  line = "global queue: ";
  line += std::to_string(engine::Engine::Get().GetRenderer().global_queue_size());
  lines.push_back(line);
  line = "render queue: ";
  line += std::to_string(engine::Engine::Get().GetRenderer().render_queue_size());
  lines.push_back(line);
  // if (!stats_.Print(engine::Engine::Get().GetFont(), lines, 300)) {
  //   LOG << "Failed to create the text.";
  // }

  constexpr int margin = 3;
  int line_height = font_.GetLineHeight();
  int image_width = width + margin * 2;
  int image_height = (line_height + margin) * lines.size() + margin;

  auto image = std::make_shared<Image>();
  image->Create(image_width, image_height);
  float c[4] = {1, 1, 1, 0.08f};
  image->Clear(c);

  int y = margin;
  for (auto& text : lines) {
    font_.Print(margin, y + margin, text.c_str(), image->GetBuffer(),
        image->GetWidth());
    y += line_height + margin;
  }

  stats_.Create(image);

  Vector2 pos = (GetScreenSize() / 2 - stats_.scale() / 2);
  pos -= Vector2(0.02f, 0.1f);
  stats_.SetOffset(pos * Vector2(-1, 1));
}

}  // namespace engine
