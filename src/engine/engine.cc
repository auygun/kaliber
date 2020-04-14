#include "../base/log.h"
#include "../base/random.h"
#include "engine.h"
#include "game.h"
#include "game_factory.h"

namespace engine {

Engine &Engine::Get() {
  static Engine engine;
  return engine;
}

bool Engine::Init() {
  RandomInit();

  if (!font.Create()) {
    LOG("Failed to create the font.\n");
    return false;
  }

  game_ = engine::GameFactoryBase::CreateGame("");
  if (!game_) {
    printf("No game found to run.\n");
    return false;
  }

  if (!game_->Initialize()) {
    LOG("Failed to initialize the game.\n");
    return false;
  }

  return true;
}

void Engine::Shutdown() {
  renderer.Shutdown();
  game_->Shutdown();
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
  // grey += 0.01f;
  // if (grey > 1.0f)
  //   grey = 0.0f;
  const float clearColor[] = { grey, grey, grey, 1.0f };
  renderer.Clear(clearColor);
}

void Engine::Present() {
  renderer.Present();
}

void Engine::TrimMemory() {
  renderer.TrimMemory();
}

} // namespace engine
