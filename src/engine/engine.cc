#include "engine.h"
#include "../base/log.h"
#include "../base/random.h"
#include "game.h"
#include "game_factory.h"

namespace engine {

Engine& Engine::Get() {
  static Engine engine;
  return engine;
}

bool Engine::Init() {
  RandomInit();

  if (!font_.Create()) {
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

  GetRenderer().Sync();

  return true;
}

void Engine::Shutdown() {
  game_.reset();
}

void Engine::Update(float delta_time) {
  game_->Update(delta_time);
}

void Engine::Draw(float frame_frac) {
  Clear();
  game_->Draw(frame_frac);
  Present();
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
