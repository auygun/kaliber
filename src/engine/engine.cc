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

  stats_.Translate({-0.74, 0.9});

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
}

void Engine::Draw(float frame_frac) {
  renderer_.EnterDrawStage();
  Clear();
  renderer_.EnableBlend();
  // game_->Draw(frame_frac);
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

Vector2 Engine::ToScale(int width, int height) {
  float horizontal_ratio =
      (float)width / GetRenderer().GetScreenWidth();
  float vertical_ratio =
      (float)height / GetRenderer().GetScreenHeight();

  // The orthogonal viewport is (-1.0 .. 1.0) x (-1.0 .. 1.0).
  return Vector2(horizontal_ratio * 2.0f, vertical_ratio * 2.0f);
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
