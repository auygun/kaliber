#include "platform.h"
#include "../base/timer.h"
#include "../engine/engine.h"

Platform::InternalError Platform::internal_error;

Platform& Platform::Get() {
  static Platform platform;
  return platform;
}

void Platform::RunMainLoop() {
  if (!engine::Engine::Get().Init()) {
    printf("Failed to initialize the engine.\n");
    throw internal_error;
  }

  // Use fixed time steps.
  constexpr float time_step = 1.0f / 60.0f;
  constexpr float speed = 1.0f;

  Timer timer;
  float last_time = timer.GetSecondsAccumulated();
  float accumulator = 0.0;
  float frame_frac = 0.0f;

  for (;;)
  {
    engine::Engine::Get().Draw(frame_frac);

    Update();

    if (ShouldExit()) {
      engine::Engine::Get().Shutdown();
      return;
    }

    timer.Update();
    float new_time = timer.GetSecondsAccumulated();
    float frame_time = (new_time - last_time) * speed;
    last_time = new_time;
    accumulator += frame_time;

    // Subdivide the frame time.
    while (accumulator >= time_step)
    {
      engine::Engine::Get().Update(time_step);
      accumulator -= time_step;
    }

    // Calculate frame fraction from remainder of the frame time.
    frame_frac = accumulator / time_step;
  }
}
