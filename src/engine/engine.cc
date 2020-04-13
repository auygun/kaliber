#include "../base/log.h"
#include "../base/random.h"
#include "engine.h"
#include "engine_config.h"
#include "game.h"
#include "game_factory.h"

namespace engine {

Engine &Engine::Get() {
  static Engine engine;
  return engine;
}

#if defined(__ANDROID__)

bool Engine::Init(ANativeWindow* window) {
  RandomInit();

  if (!timer.Init()) {
    LOG("Failed to initalize the timer.\n");
    return false;
  }

  if (!renderer.Init(window)) {
    LOG("Failed to initialize the renderer.\n");
    return false;
  }

  if (!font.Create()) {
    LOG("Failed to create the font.\n");
    return false;
  }

  return true;
}

#else

bool Engine::Init(const EngineConfig &config) {
  RandomInit();

  if (!timer.Init()) {
    LOG("Failed to initalize the timer.\n");
    return false;
  }

  if (!renderer.Init(config.screen_width, config.screen_height)) {
    LOG("Failed to initialize the renderer.\n");
    return false;
  }

  if (!font.Create()) {
    LOG("Failed to create the font.\n");
    return false;
  }

  return true;
}

#endif

void Engine::Shutdown() {
  renderer.Shutdown();
}

void Engine::Update() {
  timer.Update();
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

// Static
int Engine::Run() {
#if !defined(__ANDROID__)

  std::unique_ptr<Game> game = GameFactoryBase::CreateGame("");
  if (!game) {
    printf("No game found to run.\n");
    return 1;
  }

  if (!Engine::Get().Init(game->GetEngineConfig())) {
    printf("Failed to initialize the engine.\n");
    return 1;
  }

  if (!game->Initialize()) {
    printf("Failed to initialize the game.\n");
    return 1;
  }

  // Use fixed time steps.
  constexpr float time_step = 1.0f / 60.0f;
  constexpr float speed = 1.0f;
  float last_time = Engine::Get().GetTimer().GetSecondsAccumulated();
  float accumulator = 0.0;
  float frame_frac = 0.0f;

  for (;;)
  {
    Engine::Get().Clear();
    game->Draw(frame_frac);
    Engine::Get().Present();

    Engine::Get().Update();

    float new_time = Engine::Get().GetTimer().GetSecondsAccumulated();
    float frame_time = (new_time - last_time) * speed;
    last_time = new_time;
    accumulator += frame_time;

    // Subdivide the frame time.
    while (accumulator >= time_step)
    {
      game->Update(time_step);
      accumulator -= time_step;
    }

    // Calculate frame fraction from remainder of the frame time.
    frame_frac = accumulator / time_step;

    // if (m_platform->ShouldExit())
    //   break;
  }

  game->Shutdown();

#endif

  return 0;
}

} // namespace engine
