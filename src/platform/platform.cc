#include "platform.h"
#include "../base/log.h"
#include "../base/timer.h"
#include "../engine/engine.h"
#include <math.h>
#include <thread>

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
  float new_time = 0.0f;
  float frame_time = 0.0f;
  float accumulator = 0.0;
  float frame_frac = 0.0f;

  bool should_draw = true;
  for (;;) {
    // Save battery for mobile devices.
    if (should_draw) {
      should_draw = false;
      engine::Engine::Get().Draw(frame_frac);

      Update();
      if (should_exit_) {
        engine::Engine::Get().Shutdown();
        return;
      }
    }

    // Accumulate time in case world updates too fast.
    for (;;) {
      timer.Update();
      new_time = timer.GetSecondsAccumulated();
      frame_time = new_time - last_time;
      if (frame_time > 0.00001)
        break;
      std::this_thread::sleep_for(std::chrono::microseconds(10));
    }
    last_time = new_time;
    accumulator += frame_time;
    // LOG("accumulator: %f\n", accumulator);

    // Subdivide the frame time.
    while (accumulator >= time_step) {
      engine::Engine::Get().Update(time_step * speed);
      accumulator -= time_step;
      should_draw = true;
    };

    // Calculate frame fraction from remainder of the frame time.
    frame_frac = accumulator / time_step;
  }
}
