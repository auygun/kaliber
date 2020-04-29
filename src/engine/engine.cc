#include "engine.h"
#include "../base/log.h"
#include "../base/random.h"
#include "game.h"
#include "game_factory.h"
#include "drawable.h"
#include "image_quad.h"
#include "text_box.h"

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

  return true;
}

void Engine::Shutdown() {
  game_.reset();
}

void Engine::AddDrawable(Drawable* drawable) {
  drawables_.push_back(drawable);
}

void Engine::RemoveDrawable(Drawable* drawable) {
  // TODO: Implement.
}

void Engine::Update(float delta_time) {
  seconds_accumulated_ += delta_time;
  game_->Update(delta_time);

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
  if (!stats_.Print(engine::Engine::Get().GetFont(), lines, 300)) {
    LOG << "Failed to create the text.";
  }

  Vector2 pos = (GetScreenSize() / 2 - stats_.scale() / 2);
  pos -= Vector2(0.02f, 0.1f);
  stats_.SetOffset(pos * Vector2(-1, 1));
}

void Engine::Draw(float frame_frac) {
  renderer_.EnterDrawStage();
  Clear();
  renderer_.EnableBlend();
  for (auto d : drawables_)
    d->Draw();
  stats_.Draw();
  Present();
  renderer_.ExitDrawStage();
}

void Engine::Clear() {
  // Pulsate the clear color to make it more visible if we see it.
  static float grey = 0.0f;
  grey += 0.01f;
  if (grey > 1.0f)
    grey = 0.0f;
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

}  // namespace engine
